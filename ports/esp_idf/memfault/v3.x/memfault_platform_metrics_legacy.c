//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port of dependency functions for Memfault metrics subsystem using FreeRTOS.
//!
//! @note For test purposes, the heartbeat interval can be changed to a faster period
//! by using the following CFLAG:
//!   -DMEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS=15

#include "esp_timer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/esp_port/metrics.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "sdkconfig.h"

#if CONFIG_MEMFAULT_LWIP_METRICS
  #include "memfault/ports/lwip/metrics.h"
#endif  // CONFIG_MEMFAULT_LWIP_METRICS

#if CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS
  #include "memfault/ports/freertos/metrics.h"
#endif  // CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS

#if CONFIG_MEMFAULT_MBEDTLS_METRICS
  #include "memfault/ports/mbedtls/metrics.h"
#endif  // CONFIG_MEMFAULT_MBEDTLS_METRICS

MEMFAULT_WEAK
void memfault_esp_metric_timer_dispatch(MemfaultPlatformTimerCallback handler) {
  if (handler == NULL) {
    return;
  }
  handler();
}

static void prv_metric_timer_handler(void *arg) {
  memfault_reboot_tracking_reset_crash_count();

  // NOTE: This timer runs once per MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS where the default is
  // once per hour.
  MemfaultPlatformTimerCallback *metric_timer_handler = (MemfaultPlatformTimerCallback *)arg;
  memfault_esp_metric_timer_dispatch(metric_timer_handler);
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &prv_metric_timer_handler,
    .arg = callback,
    .name = "mflt",
  };

  // Ignore return value; this function should be safe to call multiple times
  // during system init, but needs to called before we create any timers.
  // See implementation here (may change by esp-idf version!):
  // https://github.com/espressif/esp-idf/blob/master/components/esp_timer/src/esp_timer.c#L431-L460
  (void)esp_timer_init();

  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

  const int64_t us_per_sec = 1000000;
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, period_sec * us_per_sec));

  return true;
}

void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if CONFIG_MEMFAULT_LWIP_METRICS
  memfault_lwip_heartbeat_collect_data();
#endif  // CONFIG_MEMFAULT_LWIP_METRICS

#ifdef CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS
  MEMFAULT_STATIC_ASSERT(
    MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS <= (60 * 60),
    "Heartbeat must be an hour or less for runtime metrics to mitigate counter overflow");

  memfault_freertos_port_task_runtime_metrics();
#endif  // CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS

#if CONFIG_MEMFAULT_MBEDTLS_METRICS
  memfault_mbedtls_heartbeat_collect_data();
#endif  // CONFIG_MEMFAULT_MBEDTLS_METRICS
}
