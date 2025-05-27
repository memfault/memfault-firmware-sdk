//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <time.h>

#include "rtc.h"

//! Enable the below define for debug logging
#define MEMFAULT_DEEP_SLEEP_DEBUG 0
#if defined(MEMFAULT_DEEP_SLEEP_DEBUG)
  #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "esp_log.h"
#include "esp_sleep.h"
#include "memfault/components.h"

// save a struct in RTC slow memory to preserve it across deep sleep
static RTC_NOINIT_ATTR struct MemfaultDeepSleepData {
  uint32_t magic;
  uint64_t time_since_boot_ms_at_sleep_begin;
  uint64_t rtc_us_at_sleep_begin;
} s_mflt_deep_sleep_data;
#define MEMFAULT_DEEP_SLEEP_MAGIC 0x5F3759DF

static uint64_t s_memfault_time_since_boot_offset;
static const char *TAG = "mflt_sleep";

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  const int64_t time_since_boot_us = esp_timer_get_time();
  const uint64_t time_since_boot_ms =
    (uint64_t)(time_since_boot_us / 1000) /* us per ms */ + s_memfault_time_since_boot_offset;
  return time_since_boot_ms;
}

void memfault_platform_deep_sleep_save_state(void) {
#if defined(MEMFAULT_DEEP_SLEEP_DEBUG)
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
  // This is called when the device is going to sleep
  ESP_LOGD(TAG, "😴 Deep sleep state saving");

  // Print current memfault time-since-boot-ms, and gettimeofday time too
  uint64_t time_since_boot_ms = memfault_platform_get_time_since_boot_ms();
  ESP_LOGD(TAG, "Time since boot: %" PRIu64 "ms", time_since_boot_ms);
  s_mflt_deep_sleep_data.time_since_boot_ms_at_sleep_begin = time_since_boot_ms;
  uint64_t rtc_us = esp_rtc_get_time_us();
  ESP_LOGD(TAG, "RTC time us: %" PRIu64 "", rtc_us);
  s_mflt_deep_sleep_data.rtc_us_at_sleep_begin = rtc_us;
  s_mflt_deep_sleep_data.magic = MEMFAULT_DEEP_SLEEP_MAGIC;

  // delay for a bit to allow logs to be sent
  esp_rom_delay_us(250 * 1000);
}

void memfault_platform_deep_sleep_restore_state(void) {
#if defined(MEMFAULT_DEEP_SLEEP_DEBUG)
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
  ESP_LOGD(TAG, "🌅 Woke up from deep sleep, reason: %d", esp_sleep_get_wakeup_cause());

  uint64_t rtc_us = esp_rtc_get_time_us();

  ESP_LOGD(TAG, "RTC time us at wakeup: %lld", rtc_us);

  if (s_mflt_deep_sleep_data.magic == MEMFAULT_DEEP_SLEEP_MAGIC) {
    ESP_LOGD(TAG, "RTC time us at sleep: %" PRIu64 "",
             s_mflt_deep_sleep_data.rtc_us_at_sleep_begin);
    // compute time in sleep from RTC delta. compensate by the current elapsed
    // time since boot to get time in sleep only.
    uint64_t time_in_sleep_ms =
      (rtc_us - s_mflt_deep_sleep_data.rtc_us_at_sleep_begin - esp_timer_get_time()) /
      1000 /* us per ms */;
    ESP_LOGD(TAG, "Time in sleep: %" PRIu64 "ms", time_in_sleep_ms);

    // set s_memfault_time_since_boot_offset to include:
    // time since boot at sleep begin + time in sleep
    s_memfault_time_since_boot_offset =
      s_mflt_deep_sleep_data.time_since_boot_ms_at_sleep_begin + time_in_sleep_ms;

    ESP_LOGD(TAG, "Time since boot offset: %" PRIu64 "ms", s_memfault_time_since_boot_offset);
  } else {
    ESP_LOGW(TAG, "Invalid deep sleep data, not restoring");
  }

  // clear the magic number
  s_mflt_deep_sleep_data.magic = 0;
}
