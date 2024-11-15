//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief Provides a built-in HTTP periodic upload task

#include <inttypes.h>
#include <stdint.h>

#include "esp_idf_version.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "memfault/config.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/esp_port/http_client.h"

#define PERIODIC_UPLOAD_TASK_NAME "mflt_periodic_upload"

#if !defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
// Runtime configurable
static bool s_mflt_upload_logs;
#endif

#if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_OTA)
  // Set default callbacks. Ideally we'd use a weak symbol here instead of a
  // Kconfig, but that's complicated due to ESP-IDF linking components as static
  // libraries.
  #if !defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_OTA_CUSTOM_CBS)
static bool prv_handle_ota_upload_available(void *user_ctx) {
  MEMFAULT_LOG_INFO("Starting OTA download ...");

  return true;
}

static bool prv_handle_ota_download_complete(void *user_ctx) {
  MEMFAULT_LOG_INFO("OTA Update Complete, Rebooting System");

  MEMFAULT_REBOOT_MARK_RESET_IMMINENT(kMfltRebootReason_FirmwareUpdate);

  esp_restart();
  return true;
}
sMemfaultOtaUpdateHandler g_memfault_ota_update_handler = {
  .user_ctx = NULL,
  .handle_update_available = prv_handle_ota_upload_available,
  .handle_download_complete = prv_handle_ota_download_complete,
  .handle_ota_done = NULL,
};
  #endif  // !CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_OTA_CUSTOM_CBS

static void prv_do_periodic_ota(void) {
  memfault_esp_port_ota_update(&g_memfault_ota_update_handler);
}
#endif  // CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_OTA

// Periodically post any Memfault data that has not yet been posted.
static void prv_periodic_upload_task(void *args) {
  const uint32_t interval_secs = CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS;

  // Randomize the first post to spread out the reporting of
  // information from a fleet of devices that all reboot at once.
  // For very low values of CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS
  // cut minimum delay of first post for better testing/demoing experience
  // For intervals below 60s, delay first post by 5s + random portion of interval
  // For larger intervals, delay first post by 60s + random portion of interval
  const uint32_t duration_secs_minimum = interval_secs >= 60 ? 60 : 5;
  const uint32_t duration_secs = duration_secs_minimum + (esp_random() % interval_secs);

  const TickType_t initial_delay_ms_ticks = (1000 * duration_secs) / portTICK_PERIOD_MS;
  const TickType_t interval_ms_ticks = (1000 * interval_secs) / portTICK_PERIOD_MS;

  MEMFAULT_LOG_INFO("periodic_upload task up, initial delay=%" PRIu32 "s period=%" PRIu32 "s",
                    initial_delay_ms_ticks, interval_ms_ticks);

  vTaskDelay(initial_delay_ms_ticks);

  while (true) {
    if (memfault_esp_port_netif_connected()) {
#if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
      memfault_log_trigger_collection();
#else
      if (s_mflt_upload_logs) {
        memfault_log_trigger_collection();
      }
#endif

      int err = memfault_esp_port_http_client_post_data();
      if (err) {
        MEMFAULT_LOG_ERROR("Post data failed: %d", err);
      }

#if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_OTA)
      prv_do_periodic_ota();
#endif
    }

    // sleep
    vTaskDelay(interval_ms_ticks);
  }
}

#if !defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
void memfault_esp_port_http_periodic_upload_logs(bool enable) {
  s_mflt_upload_logs = enable;
}
#endif

void memfault_esp_port_http_periodic_upload_start(void) {
#if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_TASK_STATIC)
  static StackType_t s_periodic_upload_stack[CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_STACK_SIZE] = {
    0
  };
  static StaticTask_t s_periodic_upload_task = { 0 };
  TaskHandle_t result = xTaskCreateStatic(prv_periodic_upload_task, PERIODIC_UPLOAD_TASK_NAME,
                                          CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_STACK_SIZE, NULL,
                                          CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_PRIORITY,
                                          s_periodic_upload_stack, &s_periodic_upload_task);
  if (result == NULL) {
    MEMFAULT_LOG_ERROR("Periodic upload static task creation failed");
  }
#else
  BaseType_t result = xTaskCreate(prv_periodic_upload_task, PERIODIC_UPLOAD_TASK_NAME,
                                  CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_STACK_SIZE, NULL,
                                  CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_PRIORITY, NULL);
  if (result != pdTRUE) {
    MEMFAULT_LOG_ERROR("Periodic upload dynamic task creation failed");
  }
#endif
}
