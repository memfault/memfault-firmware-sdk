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
#if KERNEL_VERSION_MAJOR >= 2 && CONFIG_MEMFAULT_USE_GNU_BUILD_ID
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

#if CONFIG_MEMFAULT_CACHE_FAULT_REGS
// Map Zephyr config to Memfault define so that we can
// collect the HW fault regs before Zephyr modifies them.
#define MEMFAULT_CACHE_FAULT_REGS 1
#endif

#if CONFIG_MEMFAULT_HEAP_STATS
// Map Zephyr config to Memfault define to enable heap
// tracing collection.
#define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 1
#endif

#if CONFIG_MEMFAULT_NRF_CONNECT_SDK

#define MEMFAULT_HTTP_CHUNKS_API_HOST "chunks-nrf.memfault.com"
#define MEMFAULT_HTTP_DEVICE_API_HOST "device-nrf.memfault.com"

#endif

#if defined(CONFIG_MEMFAULT_FAULT_HANDLER_RETURN)
#define MEMFAULT_FAULT_HANDLER_RETURN 1
#endif

#if defined(CONFIG_MEMFAULT_COMPACT_LOG)
#define MEMFAULT_COMPACT_LOG_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_SYNC_MEMFAULT_METRICS)
#define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 1
#endif

#if CONFIG_MEMFAULT_USER_CONFIG_ENABLE

// Pick up any user configuration overrides. This should be kept at the bottom
// of this file
#if CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL

# if __has_include("memfault_platform_config.h")
#   include "memfault_platform_config.h"
# endif

#else

#include "memfault_platform_config.h"

#endif /* CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL */

#else /* ! CONFIG_MEMFAULT_USER_CONFIG_ENABLE */
#define MEMFAULT_DISABLE_USER_TRACE_REASONS 1
#endif /* CONFIG_MEMFAULT_USER_CONFIG_ENABLE */

// Pick up any extra configuration settings
#if defined(CONFIG_MEMFAULT_PLATFORM_EXTRA_CONFIG_FILE)
#include "memfault_platform_config_extra.h"
#endif

#ifdef __cplusplus
}
#endif
