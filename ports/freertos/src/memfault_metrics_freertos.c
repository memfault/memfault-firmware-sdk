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


#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"

#include "FreeRTOS.h"
#include "timers.h"

static MemfaultPlatformTimerCallback *s_metric_timer_cb = NULL;
static void prv_metric_timer_callback(TimerHandle_t handle) {
  s_metric_timer_cb();
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  TimerHandle_t timer = xTimerCreate(
      "metric_timer",
      pdMS_TO_TICKS(period_sec * 1000),
      pdTRUE, /* auto reload */
      (void*)NULL,
      prv_metric_timer_callback);
  if (timer == 0) {
    return false;
  }

  s_metric_timer_cb = callback;

  xTimerStart(timer, 0);
  return true;
}
