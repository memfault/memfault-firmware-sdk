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

#include "FreeRTOS.h"
#include "memfault/core/compiler.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/freertos.h"
#include "timers.h"

// Define portTICK_PERIOD_MS is using an older version of FreeRTOS for compatibility
// portTICK_PERIOD_MS was added in FreeRTOS v8.0.0
#ifndef portTICK_PERIOD_MS
  #define portTICK_PERIOD_MS portTICK_RATE_MS
#endif

#define SEC_TO_FREERTOS_TICKS(period_sec) \
  ((uint64_t)(((uint64_t)period_sec * 1000ULL) / (uint64_t)portTICK_PERIOD_MS))

static MemfaultPlatformTimerCallback *s_metric_timer_cb = NULL;
static void prv_metric_timer_callback(MEMFAULT_UNUSED TimerHandle_t handle) {
  s_metric_timer_cb();
}

static TimerHandle_t prv_metric_timer_init(const char *const pcTimerName,
                                           TickType_t xTimerPeriodInTicks, UBaseType_t uxAutoReload,
                                           void *pvTimerID,
                                           TimerCallbackFunction_t pxCallbackFunction) {
#if MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticTimer_t s_metric_timer_context;
  return xTimerCreateStatic(pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID,
                            pxCallbackFunction, &s_metric_timer_context);
#else
  return xTimerCreate(pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID,
                      pxCallbackFunction);
#endif
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  // Validate MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS does not overflow when converting to ticks
  // Assumes a tick rate <= 1000 Hz and MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS used as period
  MEMFAULT_STATIC_ASSERT(
    MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS <= (uint32_t)(UINT32_MAX / 1000UL),
    "Period too large and will cause overflow");

  TimerHandle_t timer = prv_metric_timer_init("metric_timer", SEC_TO_FREERTOS_TICKS(period_sec),
                                              pdTRUE, /* auto reload */
                                              (void *)NULL, prv_metric_timer_callback);
  if (timer == 0) {
    return false;
  }

  s_metric_timer_cb = callback;

  xTimerStart(timer, 0);
  return true;
}
