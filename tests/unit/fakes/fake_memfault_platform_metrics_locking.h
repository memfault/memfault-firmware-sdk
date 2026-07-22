#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Fake implementation of memfault_metrics_platform_locking APIs

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @return true if there has been an equivalent number of lock and unlock calls, else false
bool fake_memfault_platform_metrics_lock_calls_balanced(void);

//! @return the number of memfault_lock() calls observed since the last reboot
uint32_t fake_memfault_platform_metrics_lock_get_lock_count(void);

//! @return the number of memfault_unlock() calls observed since the last reboot
uint32_t fake_memfault_platform_metrics_lock_get_unlock_count(void);

//! Reset the state of the fake locking tracker
void fake_memfault_metrics_platform_locking_reboot(void);

#ifdef __cplusplus
}
#endif
