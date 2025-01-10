#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Zephyr port overrides for the default configuration settings in the memfault-firmware-sdk.

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MEMFAULT_USE_GNU_BUILD_ID)
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

#if defined(CONFIG_MEMFAULT_CACHE_FAULT_REGS)
  // Map Zephyr config to Memfault define so that we can
  // collect the HW fault regs before Zephyr modifies them.
  #define MEMFAULT_CACHE_FAULT_REGS 1
#endif

#if defined(CONFIG_MEMFAULT_HEAP_STATS)
  // Map Zephyr config to Memfault define to enable heap
  // tracing collection.
  #define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 1
#endif

#if defined(CONFIG_MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE)
  #define MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_NRF_CONNECT_SDK)

  #define MEMFAULT_HTTP_CHUNKS_API_HOST "chunks-nrf.memfault.com"
  #define MEMFAULT_HTTP_DEVICE_API_HOST "device-nrf.memfault.com"

#endif

#if defined(CONFIG_MEMFAULT_FAULT_HANDLER_RETURN)
  #define MEMFAULT_FAULT_HANDLER_RETURN 1
#endif

#if defined(CONFIG_MEMFAULT_COMPACT_LOG)
  #define MEMFAULT_COMPACT_LOG_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_SYNC_SUCCESS)
  #define MEMFAULT_METRICS_SYNC_SUCCESS 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
  #define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME)
  #define MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_BATTERY_ENABLE)
  #define MEMFAULT_METRICS_BATTERY_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_SHELL_SELF_TEST)
  #define MEMFAULT_DEMO_CLI_SELF_TEST 1
  #define MEMFAULT_SELF_TEST_COREDUMP_STORAGE_DISABLE_MSG \
    "Set CONFIG_MEMFAULT_SHELL_SELF_TEST_COREDUMP_STORAGE in your prj.conf"
#endif

#if defined(CONFIG_MEMFAULT_SHELL_SELF_TEST_COREDUMP_STORAGE)
  #define MEMFAULT_DEMO_CLI_SELF_TEST_COREDUMP_STORAGE 1
#endif

#if defined(CONFIG_MEMFAULT_CDR_ENABLE)
  #define MEMFAULT_CDR_ENABLE 1
#endif

// Defaults to 1, so only disable if Kconfig setting is unset
#if !defined(CONFIG_MEMFAULT_METRICS_LOGS_ENABLE)
  #define MEMFAULT_METRICS_LOGS_ENABLE 0
#endif

#if defined(CONFIG_MEMFAULT_COREDUMP_COLLECT_MPU_STATE)
  #define MEMFAULT_COLLECT_MPU_STATE 1
#endif

#if defined(CONFIG_MEMFAULT_COREDUMP_MPU_REGIONS_TO_COLLECT)
  #define MEMFAULT_MPU_REGIONS_TO_COLLECT CONFIG_MEMFAULT_COREDUMP_MPU_REGIONS_TO_COLLECT
#endif

#if defined(CONFIG_MEMFAULT_USER_CONFIG_ENABLE)

  // Pick up any user configuration overrides. This should be kept at the bottom
  // of this file
  #if defined(CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL)

    #if __has_include("memfault_platform_config.h")
      #include "memfault_platform_config.h"
    #endif

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

#if defined(CONFIG_MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT)
  #define MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT 1
#endif

#if defined(MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS)
  #error \
    "MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS is deprecated. Use CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS instead."
#endif

#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS

#ifdef __cplusplus
}
#endif
