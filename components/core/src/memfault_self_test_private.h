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

//! Macro for beginning of test string
#define MEMFAULT_SELF_TEST_END_OUTPUT "=============================\n"
//! Macro for end of test string
#define MEMFAULT_SELF_TEST_BEGIN_OUTPUT "============================="

#define MEMFAULT_SELF_TEST_PRINT_HEADER(test_name)      \
  do {                                                  \
    MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_BEGIN_OUTPUT); \
    MEMFAULT_LOG_INFO(test_name);                       \
    MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_BEGIN_OUTPUT); \
  } while (0)

// Checks if char fits regex: [-a-zA-z0-0_]
bool memfault_self_test_valid_device_serial(unsigned char c);

//! Checks if char fits regex: [-a-zA-Z0-9_\.\+:]
bool memfault_self_test_valid_hw_version_sw_type(unsigned char c);

//! Simple wrapper for isprint, software version does not have any regex to match
bool memfault_self_test_valid_sw_version(unsigned char c);

//! Runs device info and build ID test
uint32_t memfault_self_test_device_info_test(void);

//! Runs component boot test
void memfault_self_test_component_boot_test(void);

//! Data export test
void memfault_self_test_data_export_test(void);

//! Runs coredump regions test
uint32_t memfault_self_test_coredump_regions_test(void);

#ifdef __cplusplus
}
#endif
