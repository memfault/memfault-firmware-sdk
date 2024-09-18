#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#define MEMFAULT_USE_GNU_BUILD_ID 1
#define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1
#define MEMFAULT_COMPACT_LOG_ENABLE 1

// Enable capture of entire ISR state at time of crash
#define MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT 64
