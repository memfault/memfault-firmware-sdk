//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Test functions for exercising memfault functionality.
//!

#include "memfault_test.h"

#include "memfault/components.h"
#include "memfault/ports/freertos.h"

#define MEMFAULT_CHUNK_SIZE 1500

// Triggers an immediate heartbeat capture (instead of waiting for timer
// to expire)
int memfault_test_heartbeat(int argc, char *argv[]) {
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
}

int memfault_test_trace(int argc, char *argv[]) {
  MEMFAULT_TRACE_EVENT_WITH_LOG(critical_error, "A test error trace!");
  return 0;
}

//! Trigger a user initiated reboot and confirm reason is persisted
int memfault_test_reboot(int argc, char *argv[]) {
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
}

//
// Test different crash types where a coredump should be captured
//

int memfault_test_assert(int argc, char *argv[]) {
  MEMFAULT_ASSERT(0);
  return -1;  // should never get here
}

int memfault_test_fault(void) {
  void (*bad_func)(void) = (void *)0xEEEEDEAD;
  bad_func();
  return -1;  // should never get here
}

int memfault_test_hang(int argc, char *argv[]) {
  while (1) {
  }
  return -1;  // should never get here
}

// Dump Memfault data collected to console
int memfault_test_export(int argc, char *argv[]) {
  memfault_data_export_dump_chunks();
  return 0;
}
