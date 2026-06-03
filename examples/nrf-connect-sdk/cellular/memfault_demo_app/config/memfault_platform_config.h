#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

// Allow longer debug log lines (e.g. for printing out full OTA URLs in the logs)
#define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES 400
