#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Optional platform-specific boot function for initializing connectivity metrics.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The platform must implement this function. It is called during boot to initialize
//! platform-specific connectivity metrics.
//!
//! For example, this function may register handlers for connectivity events, which when
//! received, the handlers will mark the begin and end of connection periods.
void memfault_platform_metrics_connectivity_boot(void);

#ifdef __cplusplus
}
#endif
