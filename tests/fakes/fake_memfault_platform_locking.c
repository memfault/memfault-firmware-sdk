//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of memfault_metrics_platform_locking APIs

#include "fakes/fake_memfault_platform_metrics_locking.h"

#include <stdint.h>

#include "memfault/core/platform/overrides.h"

typedef struct {
  uint32_t lock_count;
  uint32_t unlock_count;
} sMetricLockStats;

static sMetricLockStats s_metric_lock_stats;

void memfault_lock(void) {
  s_metric_lock_stats.lock_count++;
}

void memfault_unlock(void) {
  s_metric_lock_stats.unlock_count++;
}

void fake_memfault_metrics_platorm_locking_reboot(void) {
    s_metric_lock_stats = (sMetricLockStats) { 0 };
}

bool fake_memfault_platform_metrics_lock_calls_balanced(void) {
  return s_metric_lock_stats.lock_count == s_metric_lock_stats.unlock_count;
}
