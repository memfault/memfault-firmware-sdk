#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#define MEMFAULT_TASK_WATCHDOG_ENABLE 1
#define MEMFAULT_COMPACT_LOG_ENABLE 1

// Enable the sync_successful metric
#define MEMFAULT_METRICS_SYNC_SUCCESS 1
