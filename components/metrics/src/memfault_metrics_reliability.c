//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief APIs for tracking reliability metrics

#include "memfault/metrics/reliability.h"

// non-module includes below

#include "memfault/components.h"

// Accumulate operational hours.
void memfault_metrics_reliability_collect(void) {
  static uint32_t s_memfault_last_heartbeat_ms = 0;
  static uint32_t s_memfault_operational_ms = 0;
  static bool s_memfault_counted_unexpected_reboot = false;

  // Accumulate ms since last heartbeat
  const uint32_t time_since_boot_ms = memfault_platform_get_time_since_boot_ms();
  const uint32_t this_interval_ms = time_since_boot_ms - s_memfault_last_heartbeat_ms;
  s_memfault_operational_ms += this_interval_ms;

  // Count any accumulated hours
  const uint32_t uptime_hours = s_memfault_operational_ms / 1000 / 3600;
  // Deduct the counted hours from the accumulated ms
  s_memfault_operational_ms -= uptime_hours * 3600 * 1000;
  // Reset the accumulated ms for the next heartbeat
  s_memfault_last_heartbeat_ms = time_since_boot_ms;

  // Only report uptime if it's non-zero
  if (!uptime_hours) {
    return;
  }

  MEMFAULT_METRIC_ADD(operational_hours, (int32_t)uptime_hours);

  bool unexpected_reboot = false;
  if (!s_memfault_counted_unexpected_reboot) {
    s_memfault_counted_unexpected_reboot = true;
    (void)memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  }
  // Deduct an hour from the crashfree hours count if there was an unexpected
  // reboot. This only applies to the first hour of uptime, which is where the
  // crash is counted.
  MEMFAULT_METRIC_ADD(operational_crashfree_hours,
                         (int32_t)(uptime_hours - (uint32_t)unexpected_reboot));
}
