//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/core/reboot_tracking.h"

void memfault_reboot_tracking_metrics_session(bool active, uint32_t index) {
  (void)active;
  (void)index;
}

bool memfault_reboot_tracking_metrics_session_was_active(uint32_t index) {
  (void)index;
  return false;
}

void memfault_reboot_tracking_clear_metrics_sessions(void) { }
