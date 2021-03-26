#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Zephyr port overrides for the default configuration settings in the memfault-firmware-sdk.
#include <autoconf.h> // For Kconfig settings
#include <version.h>  // Zephyr version macros

#ifdef __cplusplus
extern "C" {
#endif

// Note that pre-v2.0 Zephyr did not create the section allocation needed to support
// our Gnu build ID usage.
#if KERNEL_VERSION_MAJOR >= 2
// Add a unique identifier to the firmware build
//
// It is very common, especially during development, to not change the firmware
// version between editing and compiling the code. This will lead to issues when
// recovering backtraces or symbol information because the debug information in
// the symbol file may be out of sync with the actual binary. Tracking a build id
// enables the Memfault cloud to identify and surface when this happens! Below
// requires the "-Wl,--build-id" flag.
#define MEMFAULT_USE_GNU_BUILD_ID 1
#endif

// We need to define MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS=1 for the logs to
// show up in the Memfault UI on crash.
#ifndef MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS
#define MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS 1
#endif

#define MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS CONFIG_MEMFAULT_SOFTWARE_WATCHDOG_TIMEOUT_SECS

// Logs are saved to the Memfault logging system as part of
// memfault logging integration (CONFIG_MEMFAULT_LOGGING_ENABLE=y)
// so no need to save from the SDK
#define MEMFAULT_SDK_LOG_SAVE_DISABLE 1

// Pick up any user configuration overrides
#include "memfault_platform_config.h"

#ifdef __cplusplus
}
#endif
