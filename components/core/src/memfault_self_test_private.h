//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Checks if char fits regex: [-a-zA-z0-0_]
bool memfault_self_test_valid_device_serial(unsigned char c);

//! Checks if char fits regex: [-a-zA-Z0-9_\.\+:]
bool memfault_self_test_valid_hw_version_sw_type(unsigned char c);

//! Simple wrapper for isprint, software version does not have any regex to match
bool memfault_self_test_valid_sw_version(unsigned char c);

//! Runs device info and build ID test
uint32_t memfault_self_test_device_info_test(void);

#ifdef __cplusplus
}
#endif
