//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memfault/core/compiler.h"

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
uint32_t memfault_self_test_component_boot_test(void);

//! Data export test
void memfault_self_test_data_export_test(void);

//! Runs coredump regions test
uint32_t memfault_self_test_coredump_regions_test(void);

//! Runs reboot reason test
//!
//! This function begins the test by setting a known reboot reason and then rebooting the device
//! After the device reboots, memfault_self_test_reboot_reason_test_verify should be used to verify
//! the test results
MEMFAULT_NORETURN void memfault_self_test_reboot_reason_test(void);

//! Verifies reboot reason test results
uint32_t memfault_self_test_reboot_reason_test_verify(void);

//! Runs tests against platform time functions
uint32_t memfault_self_test_time_test(void);

//! Runs tests against platform coredump storage implementation
//!
//! This test will check for the existence of a valid coredump before proceeding and abort if one is
//! found to prevent overwriting valid data
uint32_t memfault_self_test_coredump_storage_test(void);

//! Runs test to check capacity of coredump storage against worst case
uint32_t memfault_self_test_coredump_storage_capacity_test(void);

//! Internal implementation of strnlen
//!
//! Support for strnlen is inconsistent across a lot of libc implementations so we implement this
//! here for compatibility reasons. See
//! https://pubs.opengroup.org/onlinepubs/9699919799/functions/strlen.html
//! @param str Pointer to a C string. Pointer must not be NULL
//! @param n Maximum number of characters to examine when determining the length of the string
//! @returns The strnlen() function shall return the number of bytes preceding the first null byte
//! in the array to which s points, if s contains a null byte within the first maxlen bytes;
//! otherwise, it shall return maxlen
size_t memfault_strnlen(const char *str, size_t n);

#ifdef __cplusplus
}
#endif
