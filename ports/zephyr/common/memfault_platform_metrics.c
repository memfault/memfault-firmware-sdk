//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <zephyr.h>

#include <stdbool.h>

#include "memfault/metrics/platform/timer.h"

static MemfaultPlatformTimerCallback *s_metrics_timer_callback;

static void prv_metrics_work_handler(struct k_work *work) {
  if (s_metrics_timer_callback != NULL) {
    s_metrics_timer_callback();
  }
}

K_WORK_DEFINE(s_metrics_timer_work, prv_metrics_work_handler);

//! Timer handlers run from an ISR so we dispatch the heartbeat job to the worker task
static void prv_timer_expiry_handler(struct k_timer *dummy) {
  k_work_submit(&s_metrics_timer_work);
}

K_TIMER_DEFINE(s_metrics_timer, prv_timer_expiry_handler, NULL);

bool memfault_platform_metrics_timer_boot(
    uint32_t period_sec, MemfaultPlatformTimerCallback callback) {
  s_metrics_timer_callback = callback;

  k_timer_start(&s_metrics_timer, K_SECONDS(period_sec), K_SECONDS(period_sec));
  return true;
}
