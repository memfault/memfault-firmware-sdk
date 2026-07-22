#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Definitions related to the Memfault project key, which is used to
//! identify data uploaded over any transport (HTTP, BLE, CoAP, etc.)

#ifdef __cplusplus
extern "C" {
#endif

//! Length of a Memfault project key (not including null terminator).
#define MEMFAULT_PROJECT_KEY_LEN 32

#ifdef __cplusplus
}
#endif
