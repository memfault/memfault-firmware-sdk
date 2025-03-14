#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#define MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE 1

// disabled for most tests
#if !defined(MEMFAULT_LOG_TIMESTAMPS_ENABLE)
  #define MEMFAULT_LOG_TIMESTAMPS_ENABLE 0
#endif
