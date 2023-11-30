#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Required dependency functions for the Memfault Battery Metrics component.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The platform must implement this function. It provides the current battery
//! state of charge in percent (0-100%).
//!
//! @returns the current battery state of charge in percent (0-100%)
uint32_t memfault_platform_get_stateofcharge(void);

//! The platform must implement this function. The battery metrics
//! implementation uses this to check if the battery is discharging.
//!
//! @return true if the system is discharging, false if it is not discharging
bool memfault_platform_is_discharging(void);

#ifdef __cplusplus
}
#endif
