#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Functions that can be optionally overriden by the user of the SDK.
//!
//! Default "weak function" stub definitions are provided for each of these functions
//! in the SDK itself

#ifdef __cplusplus
extern "C" {
#endif

//! Locking APIs used within the Memfault SDK
//!
//! The APIs can (optionally) be overriden by the application to enable locking around
//! accesses to Memfault APIs. This is required if calls are made to Memfault APIs from
//! multiple tasks
//!
//! @note It's expected that the mutexes implemented are recursive mutexes.
void memfault_lock(void);
void memfault_unlock(void);

#ifdef __cplusplus
}
#endif
