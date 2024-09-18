//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>

//! Structure to hold mbedTLS metrics, for runtime access
typedef struct MemfaultMbedtlsMetricData {
  int32_t mem_used_bytes;
  uint32_t mem_max_bytes;
} sMemfaultMbedtlsMetricData;

//! Fetch the current MbedTLS metric data
void memfault_mbedtls_heartbeat_get_data(sMemfaultMbedtlsMetricData *metrics);

//! Collects a heartbeat of mbedTLS metrics before resetting the values
void memfault_mbedtls_heartbeat_collect_data(void);

//! Clears backing metric values, only for testing purposes
void memfault_mbedtls_test_clear_values(void);

#ifdef __cplusplus
}
#endif  // __cplusplus
