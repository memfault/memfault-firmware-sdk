//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//! Collects a heartbeat of mbedTLS metrics before resetting the values
void memfault_mbedtls_heartbeat_collect_data(void);

//! Clears backing metric values, only for testing purposes
void memfault_mbedtls_test_clear_values(void);

#ifdef __cplusplus
}
#endif  // __cplusplus
