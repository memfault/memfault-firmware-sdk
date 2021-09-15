//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Test functions for exercising memfault functionality.
//!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Trigger an immediate heartbeat capture (instead of waiting for timer
// to expire)
int memfault_test_heartbeat(int argc, char *argv[]);

// Trigger a trace
int memfault_test_trace(int argc, char *argv[]);

// Trigger a user initiated reboot and confirm reason is persisted
int memfault_test_reboot(int argc, char *argv[]);

// Test different crash types where a coredump should be captured
int memfault_test_assert(int argc, char *argv[]);
int memfault_test_fault(void);
int memfault_test_hang(int argc, char *argv[]);

// Dump Memfault data collected to console
int memfault_test_export(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
