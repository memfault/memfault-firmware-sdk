#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! MCUmgr group for Memfault device information and project key access

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Group ID for Memfault management group.
//! Using 128 as it's sufficiently higher than the start of user-defined groups (64)
#define MGMT_GROUP_ID_MEMFAULT 128

//! Command IDs for Memfault management group.
#define MEMFAULT_MGMT_ID_DEVICE_INFO 0
#define MEMFAULT_MGMT_ID_PROJECT_KEY 1

//! Command result codes for Memfault management group.
enum memfault_mgmt_err_code_t {
  // No error, this is implied if there is no ret value in the response
  MEMFAULT_MGMT_ERR_OK = 0,

  // Unknown error occurred.
  MEMFAULT_MGMT_ERR_UNKNOWN,

  // Project key not configured
  MEMFAULT_MGMT_ERR_NO_PROJECT_KEY,
};

//! @brief Callback for enabling access to the Memfault MCUmgr group. By
//! default, access is always allowed.
//!
//! @param[in] user_arg User argument provided when setting the callback.
//!
//! @retval true if access is permitted, false otherwise. if false, commands
//! will return MGMT_ERR_EACCESSDENIED.
typedef bool (*memfault_mgmt_access_cb_t)(void *user_arg);

//! @brief Sets a callback to control access to the Memfault MCUmgr group. By
//! default, access is always allowed.
//!
//! @param[in] cb Callback function to determine if access is allowed.
//! @param[in] user_arg User argument passed to the callback when invoked.
void memfault_mcumgr_set_access_callback(memfault_mgmt_access_cb_t cb, void *user_arg);

#ifdef __cplusplus
}
#endif
