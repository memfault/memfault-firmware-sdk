#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault built-in battery metrics.

#ifdef __cplusplus
extern "C" {
#endif

//! This function must be called when the battery stops discharging, eg:
//!
//! 1. charge mode is entered
//! 2. battery is disconnected
//!
//! @note This function must be called for the system to correctly report soc
//! drop and discharging time only during discharging. If it's not called
//! correctly, the metric data will be invalid.
void memfault_metrics_battery_stopped_discharging(void);

//! Record battery metric values. This is normally called automatically during
//! the end of heartbeat timer callback, and should not be called by the user.
void memfault_metrics_battery_collect_data(void);

//! Initialize the battery metrics module. Call exactly once on boot, after the
//! device's battery subsystem is initialized: the battery platform dependency
//! functions need to be valid when this function is called.
void memfault_metrics_battery_boot(void);

#ifdef __cplusplus
}
#endif
