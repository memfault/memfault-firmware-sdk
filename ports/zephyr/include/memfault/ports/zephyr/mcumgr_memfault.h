#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! MCUmgr group for Memfault device information and project key access

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Group ID for Memfault management group.
 * Using 128 as it's sufficiently higher than the start of user-defined groups (64)
 */
#define MGMT_GROUP_ID_MEMFAULT 128

/**
 * Command IDs for Memfault management group.
 */
#define MEMFAULT_MGMT_ID_DEVICE_INFO 0
#define MEMFAULT_MGMT_ID_PROJECT_KEY 1

/**
 * Command result codes for Memfault management group.
 */
enum memfault_mgmt_err_code_t {
  /** No error, this is implied if there is no ret value in the response */
  MEMFAULT_MGMT_ERR_OK = 0,

  /** Unknown error occurred. */
  MEMFAULT_MGMT_ERR_UNKNOWN,

  /** Project key not configured */
  MEMFAULT_MGMT_ERR_NO_PROJECT_KEY,
};

#ifdef __cplusplus
}
#endif
