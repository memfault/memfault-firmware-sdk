#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include <inttypes.h>

#include "memfault/metrics/platform/timer.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Called each time a heartbeat interval expires to invoke the handler
//!
//! The Memfault port implements a default implementation of this in
//! ports/esp_idf/memfault/common/memfault_platform_metrics.c which
//! runs the callback on the esp timer task.
//!
//! Since the duty cycle is low (once / hour) and the work performed is
//! copying a small amount of data in RAM, it's recommended to use the default
//! implementation. However, the function is defined as weak so an end-user
//! can override this behavior by implementing the function in their main application.
//!
//! @param handler The callback to invoke to serialize heartbeat metrics
void memfault_esp_metric_timer_dispatch(MemfaultPlatformTimerCallback handler);

typedef struct sMfltFlashCounters {
  uint32_t erase_bytes;
  uint32_t write_bytes;
} sMfltFlashCounters;

//! Collects the current flash counters and returns the difference from the last
//! time this function was called.
//!
//! @param last_counter_values The last values of the flash counters. This is
//! updated internally and should not be modified by the caller, but must be
//! initialized to 0.
//! @return The difference between the current and last values of the flash
//! counters.
//!
//! Example usage:
//! @code
//!   // static storage lifetime for the last counter values
//!   static sMfltFlashCounters last_counter_values = { 0 };
//!   sMfltFlashCounters flash_counters =
//!     memfault_platform_metrics_get_flash_counters(&last_counter_values);
//!   MEMFAULT_LOG_INFO("Flash erase bytes: %u", flash_counters.erase_bytes);
//!   MEMFAULT_LOG_INFO("Flash write bytes: %u", flash_counters.write_bytes);
//! @endcode
sMfltFlashCounters memfault_platform_metrics_get_flash_counters(
  sMfltFlashCounters *last_counter_values);

#ifdef __cplusplus
}
#endif
