//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Public interface for self_test component
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//! A bitfield-based enum used to select which self tests to run
typedef enum {
  kMemfaultSelfTestFlag_DeviceInfo = (1 << 0),
  kMemfaultSelfTestFlag_ComponentBoot = (1 << 1),
  kMemfaultSelfTestFlag_CoredumpRegions = (1 << 2),
  kMemfaultSelfTestFlag_DataExport = (1 << 3),
  kMemfaultSelfTestFlag_RebootReason = (1 << 4),
  kMemfaultSelfTestFlag_RebootReasonVerify = (1 << 5),
  kMemfaultSelfTestFlag_PlatformTime = (1 << 6),

  // A convenience mask which runs the default tests
  kMemfaultSelfTestFlag_Default =
    (kMemfaultSelfTestFlag_DeviceInfo | kMemfaultSelfTestFlag_ComponentBoot |
     kMemfaultSelfTestFlag_CoredumpRegions | kMemfaultSelfTestFlag_DataExport |
     kMemfaultSelfTestFlag_PlatformTime),
} eMemfaultSelfTestFlag;

//! Main entry point to performing a self test
//!
//! This function will carry out all component tests and return after completing all tests
//! See output for test results and feedback on any errors detected
//!
//! @returns 0 on success or 1 if a subtest failed
int memfault_self_test_run(uint32_t run_flags);

//! Translates a string arg to flag used to select which test to run
//!
//! @returns eMemfaultSelfTestFlag mapped to argument, or kMemfaultSelfTestFlag_Default if no
//! matches found.
uint32_t memfault_self_test_arg_to_flag(const char *arg);

//! Delays or puts to sleep current context for specified milliseconds
//!
//! @note This function must be implemented by your platform to perform all self tests completely
void memfault_self_test_platform_delay(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif
