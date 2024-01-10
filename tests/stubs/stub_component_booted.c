//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include <stdbool.h>

#include "memfault/core/event_storage.h"
#include "memfault/core/log.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/core/trace_event.h"

bool memfault_event_storage_booted(void) {
  return true;
}

bool memfault_log_booted(void) {
  return true;
}

bool memfault_reboot_tracking_booted(void) {
  return true;
}

bool memfault_trace_event_booted(void) {
  return true;
}
