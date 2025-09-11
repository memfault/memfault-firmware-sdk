#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#ifdef __cplusplus
extern "C" {
#endif

//! Called during heartbeat to update Bluetooth metrics
void memfault_bluetooth_metrics_heartbeat_update(void);

#ifdef __cplusplus
}
#endif
