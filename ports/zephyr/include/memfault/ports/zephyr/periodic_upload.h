#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Core Zephyr Memfault port module.

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(CONFIG_MEMFAULT_PERIODIC_UPLOAD_LOGS)
//! Enable or disable periodic log upload at runtime
void memfault_zephyr_port_periodic_upload_logs(bool enable);
#endif

//! Enable or disable periodic Memfault data uploads at runtime
//!
//! @param enable true to enable periodic uploads, false to disable
void memfault_zephyr_port_periodic_upload_enable(bool enable);

//! Gets the current state of periodic Memfault data uploads
//!
//! @return true if Memfault upload is enabled, false if disabled
bool memfault_zephyr_port_periodic_upload_enabled(void);

#ifdef __cplusplus
}
#endif
