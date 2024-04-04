//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Stubs for FreeRTOS timers.h.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t TaskHandle_t;
typedef uint32_t UBaseType_t;
typedef uint32_t StackType_t;

static TaskHandle_t xTimerGetTimerDaemonTaskHandle(void) {
  return 0;
}

static UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask) {
  (void)xTask;
  return 0;
}

#ifdef __cplusplus
}
#endif
