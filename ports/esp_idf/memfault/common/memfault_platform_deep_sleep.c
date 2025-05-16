//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <time.h>

#include "rtc.h"

#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_ENABLE_DEBUG_LOG)
  #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "esp_crc.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "memfault/components.h"
#include "memfault/esp_port/http_client.h"

// ESP-IDF < v5.0 has a known bug with RTC time. Emit a build error on earlier
// versions to prevent confusion:
// https://github.com/espressif/esp-idf/issues/9448#issuecomment-1645354025
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
  #error "ESP-IDF version < v5.0 is not supported."
#endif

// Save some state in RTC memory to preserve it across deep sleep
static RTC_NOINIT_ATTR struct MemfaultDeepSleepMetricsBackup {
  // magic number indicating the data has been backed up
  uint32_t event_storage_magic;
  // crc32 over each backup, for integrity
  uint32_t event_storage_crc32;
  // the data is stored into a single combined buffer to make it simpler to
  // manipulate
  uint8_t event_storage_data[MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES +
                             CONFIG_MEMFAULT_EVENT_STORAGE_RAM_SIZE];
  uint32_t metrics_magic;
  uint32_t metrics_crc32;
  uint8_t metrics_data[MEMFAULT_METRICS_CONTEXT_SIZE_BYTES];
  uint32_t last_time_magic;
  uint64_t heartbeat_last_time;
  uint64_t upload_last_time;
} s_mflt_metrics_backup_data;

#define MEMFAULT_DEEP_SLEEP_MAGIC 0x5F3759DF

static const char *TAG = "mflt_sleep";

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  // use the RTC timer to get the time since boot. this runs through deep sleep.
  const uint64_t time_since_boot_us = esp_rtc_get_time_us();
  const uint64_t time_since_boot_ms = (uint64_t)(time_since_boot_us / 1000) /* us per ms */;
  return time_since_boot_ms;
}

static void prv_backup_event_storage(void) {
  sMfltEventStorageSaveState state = memfault_event_storage_get_state();
  if (state.context_len == MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES &&
      state.storage_len == CONFIG_MEMFAULT_EVENT_STORAGE_RAM_SIZE) {
    memcpy(&s_mflt_metrics_backup_data.event_storage_data[0], state.context, state.context_len);
    memmove(&s_mflt_metrics_backup_data.event_storage_data[MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES],
            state.storage, state.storage_len);
  } else {
    ESP_LOGW(TAG, "Event storage state size mismatch: %d != %d or %d != %d",
             (int)MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES, (int)state.context_len,
             (int)CONFIG_MEMFAULT_EVENT_STORAGE_RAM_SIZE, (int)state.storage_len);
  }
  s_mflt_metrics_backup_data.event_storage_magic = MEMFAULT_DEEP_SLEEP_MAGIC;
  s_mflt_metrics_backup_data.event_storage_crc32 =
    esp_crc32_le(0, s_mflt_metrics_backup_data.event_storage_data,
                 sizeof(s_mflt_metrics_backup_data.event_storage_data));
  ESP_LOGD(TAG, "Event storage buf usage: %zu/%zu", memfault_event_storage_bytes_used(),
           CONFIG_MEMFAULT_EVENT_STORAGE_RAM_SIZE);
  ESP_LOGD(TAG, "Event storage backup CRC32: %08lx",
           s_mflt_metrics_backup_data.event_storage_crc32);
}

static void prv_backup_metrics(void) {
  const void *ctx = memfault_metrics_get_state();
  memcpy(&s_mflt_metrics_backup_data.metrics_data[0], ctx, MEMFAULT_METRICS_CONTEXT_SIZE_BYTES);

  s_mflt_metrics_backup_data.metrics_crc32 = esp_crc32_le(
    0, s_mflt_metrics_backup_data.metrics_data, sizeof(s_mflt_metrics_backup_data.metrics_data));
  s_mflt_metrics_backup_data.metrics_magic = MEMFAULT_DEEP_SLEEP_MAGIC;
  ESP_LOGD(TAG, "Metrics backup CRC32: %08lx", s_mflt_metrics_backup_data.metrics_crc32);
}

static void prv_check_and_trigger_heartbeat(void) {
#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_HEARTBEAT_ON_SLEEP)
  // Check if it's time to trigger a heartbeat. We do this by checking if the
  // current time-since-boot exceeds the configured heartbeat interval
  // (CONFIG)MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS); if so, call
  // memfault_metrics_heartbeat_debug_trigger(), and record the time the
  // heartbeat actually fired.
  //
  // If we assume a strictly periodic deep sleep cycle, we will end up with
  // uniform heartbeat intervals, aligned to deep sleep sleep times, of no
  // shorter than the minimum configured interval.
  const uint64_t current_time = memfault_platform_get_time_since_boot_ms();
  const uint64_t heartbeat_interval = CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS * 1000;  // ms
  const uint64_t time_since_last_heartbeat =
    current_time - s_mflt_metrics_backup_data.heartbeat_last_time;
  if (time_since_last_heartbeat >= heartbeat_interval) {
    // trigger the heartbeat collection
    ESP_LOGD(TAG, "Triggering heartbeat, time since last: %" PRIu64, time_since_last_heartbeat);
    memfault_metrics_heartbeat_debug_trigger();
    // record the time the heartbeat should have fired
    s_mflt_metrics_backup_data.heartbeat_last_time = current_time;
  }
#endif  // CONFIG_MEMFAULT_DEEP_SLEEP_HEARTBEAT_ON_SLEEP
}

static void prv_check_and_trigger_upload(void) {
#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_HTTP_UPLOAD_ON_SLEEP)
  // Check if it's time to trigger an upload. We do this by checking if the
  // current time-since-boot exceeds the configured upload interval
  // (CONFIG)MEMFAULT_DEEP_SLEEP_UPLOAD_INTERVAL_SECS); if so, call
  // memfault_platform_trigger_upload(), and record the time the upload
  // actually fired.
  //
  // If we assume a strictly periodic deep sleep cycle, we will end up with
  // uniform upload intervals, aligned to deep sleep sleep times, of no
  // shorter than the minimum configured interval.
  const uint64_t current_time = memfault_platform_get_time_since_boot_ms();
  const uint64_t upload_interval =
    CONFIG_MEMFAULT_DEEP_SLEEP_HTTP_UPLOAD_INTERVAL_SECS * 1000;  // ms
  const uint64_t time_since_last_upload =
    current_time - s_mflt_metrics_backup_data.upload_last_time;
  if (time_since_last_upload >= upload_interval) {
    // trigger the upload
    ESP_LOGD(TAG, "Triggering upload, time since last: %" PRIu64, time_since_last_upload);

  #if defined(CONFIG_MEMFAULT_DEEP_SLEEP_HTTP_UPLOAD_TRIGGER_LOGS)
    memfault_log_trigger_collection();
  #endif

    memfault_esp_port_http_client_post_data();
    s_mflt_metrics_backup_data.upload_last_time = current_time;
  }
#endif  // CONFIG_MEMFAULT_DEEP_SLEEP_HTTP_UPLOAD_ON_SLEEP
}

void memfault_platform_deep_sleep_save_state(void) {
#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_ENABLE_DEBUG_LOG)
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
  // This is called when the device is going to sleep
  ESP_LOGD(TAG, "🥱 Deep sleep state saving");

  // Check if it's time to trigger a heartbeat or upload. Since these checks
  // rely on saved state, we need to first confirm if the saved state is valid.
  // It will be invalid on the first deep sleep cycle, initialize it then.
  if (s_mflt_metrics_backup_data.last_time_magic != MEMFAULT_DEEP_SLEEP_MAGIC) {
    // Assume this is the first time we are going to deep sleep, so
    // initialize the last time values to 0.
    s_mflt_metrics_backup_data.heartbeat_last_time = 0;
    s_mflt_metrics_backup_data.upload_last_time = 0;
    s_mflt_metrics_backup_data.last_time_magic = MEMFAULT_DEEP_SLEEP_MAGIC;
  }

  prv_check_and_trigger_heartbeat();

  prv_check_and_trigger_upload();

  // Save the event storage state
  prv_backup_event_storage();

  // Save the metrics state. first trigger a collection of metrics- this will
  // include any delta computations, like network_rx/tx, etc., but does not
  // trigger any metric timer tallying- metrics timers can run through deep
  // sleep cycles.
  memfault_metrics_heartbeat_collect();

#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_METRICS)
  MEMFAULT_METRIC_TIMER_STOP(active_time_ms);
  MEMFAULT_METRIC_TIMER_START(deep_sleep_time_ms);
#endif

  prv_backup_metrics();

  ESP_LOGD(TAG, "RTC time us at sleep: %lld", esp_rtc_get_time_us());
  ESP_LOGD(TAG, "Time-since-boot ms at sleep: %lld", memfault_platform_get_time_since_boot_ms());

  // delay for a bit to allow logs to be sent
  esp_rom_delay_us(250 * 1000);
}

bool memfault_event_storage_restore_state(sMfltEventStorageSaveState *state) {
  // check magic number first
  if (s_mflt_metrics_backup_data.event_storage_magic != MEMFAULT_DEEP_SLEEP_MAGIC) {
    ESP_LOGW(TAG, "No event storage backup data");
    return false;
  }

  // check crc32
  uint32_t crc32 = esp_crc32_le(0, s_mflt_metrics_backup_data.event_storage_data,
                                sizeof(s_mflt_metrics_backup_data.event_storage_data));
  if (crc32 != s_mflt_metrics_backup_data.event_storage_crc32) {
    ESP_LOGW(TAG, "Event storage backup CRC32 mismatch: %08lx != %08lx", crc32,
             s_mflt_metrics_backup_data.event_storage_crc32);
    return false;
  }
  // copy the data back to the state
  state->context = &s_mflt_metrics_backup_data.event_storage_data[0];
  state->context_len = MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES;
  state->storage =
    &s_mflt_metrics_backup_data.event_storage_data[MEMFAULT_EVENT_STORAGE_STATE_SIZE_BYTES];
  state->storage_len = CONFIG_MEMFAULT_EVENT_STORAGE_RAM_SIZE;

  // clear the magic number; we only want to restore the backup once
  s_mflt_metrics_backup_data.event_storage_magic = 0;

  return true;
}

bool memfault_metrics_restore_state(void *state) {
  // check magic number first
  if (s_mflt_metrics_backup_data.metrics_magic != MEMFAULT_DEEP_SLEEP_MAGIC) {
    ESP_LOGW(TAG, "No metrics backup data");
    return false;
  }

  // check crc32
  uint32_t crc32 = esp_crc32_le(0, s_mflt_metrics_backup_data.metrics_data,
                                sizeof(s_mflt_metrics_backup_data.metrics_data));
  if (crc32 != s_mflt_metrics_backup_data.metrics_crc32) {
    ESP_LOGW(TAG, "Metrics backup CRC32 mismatch: %08lx != %08lx", crc32,
             s_mflt_metrics_backup_data.metrics_crc32);
    return false;
  }
  // copy the data back to the state
  memcpy(state, &s_mflt_metrics_backup_data.metrics_data[0], MEMFAULT_METRICS_CONTEXT_SIZE_BYTES);

  // clear the magic number; we only want to restore the backup once
  s_mflt_metrics_backup_data.metrics_magic = 0;

#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_METRICS)
  // there will be a few ms of error in the sleep time metric, due to bootup time.
  MEMFAULT_METRIC_TIMER_STOP(deep_sleep_time_ms);
  MEMFAULT_METRIC_TIMER_START(active_time_ms);
  MEMFAULT_METRIC_ADD(deep_sleep_wakeup_count, 1);
#endif

  return true;
}

void memfault_platform_deep_sleep_restore_state(void) {
#if defined(CONFIG_MEMFAULT_DEEP_SLEEP_ENABLE_DEBUG_LOG)
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
  ESP_LOGD(TAG, "🌅 Woke up from deep sleep, reason: %d", esp_sleep_get_wakeup_cause());

  ESP_LOGD(TAG, "RTC time us at wakeup: %lld", esp_rtc_get_time_us());
  ESP_LOGD(TAG, "Time-since-boot ms at wakeup: %lld", memfault_platform_get_time_since_boot_ms());
}
