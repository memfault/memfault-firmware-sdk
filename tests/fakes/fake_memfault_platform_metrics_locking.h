#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of memfault_metrics_platform_locking APIs

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @return true if there has been an equivalent number of lock and unlock calls, else false
bool fake_memfault_platform_metrics_lock_calls_balanced(void);

//! Reset the state of the fake locking tracker
void fake_memfault_metrics_platorm_locking_reboot(void);

#ifdef __cplusplus
}
#endif
