#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//
// ESP-IDF specific Memfault configs. This file has the following purposes:
// 1. provide a way to set Memfault configs (default_config.h overrides) from
//    ESP-IDF Kconfig flags
// 2. remove the requirement for a user to provide "memfault_platform_config.h"
//    themselves, if they don't need to override any default options

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"

#if defined(CONFIG_MEMFAULT_METRICS_SYNC_SUCCESS)
  #define MEMFAULT_METRICS_SYNC_SUCCESS 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
  #define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME)
  #define MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME 1
#endif

#if defined(CONFIG_MEMFAULT_METRICS_BATTERY)
  #define MEMFAULT_METRICS_BATTERY_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_CLI_SELF_TEST)
  #define MEMFAULT_DEMO_CLI_SELF_TEST 1
  #define MEMFAULT_SELF_TEST_COREDUMP_STORAGE_DISABLE_MSG \
    "Set CONFIG_MEMFAULT_CLI_SELF_TEST_COREDUMP_STORAGE in your prj.conf"
#endif

#if defined(CONFIG_MEMFAULT_CLI_SELF_TEST_COREDUMP_STORAGE)
  #define MEMFAULT_DEMO_CLI_SELF_TEST_COREDUMP_STORAGE 1
#endif

#if defined(CONFIG_MEMFAULT_ESP_WIFI_CONNECTIVITY_TIME_METRICS)
  #define MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME 1
#endif

#if defined(CONFIG_MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT)
  #define MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT 1
#endif

#if defined(CONFIG_MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE)
  #define MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_COMPACT_LOG_ENABLE)
  #define MEMFAULT_COMPACT_LOG_ENABLE 1
#endif

#if defined(CONFIG_MEMFAULT_HEAP_STATS)
  #define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 1
  #define MEMFAULT_HEAP_STATS_MAX_COUNT CONFIG_MEMFAULT_HEAP_STATS_MAX_COUNT
  // Synchronization is done outside of the heap stats module
  #define MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE 0
  // In-use block tallying is done manually
  #define MEMFAULT_IN_USE_BLOCK_COUNT_AUTOMATIC 0
#endif

#if defined(CONFIG_MEMFAULT_ENABLE_REBOOT_DIAG_DUMP)
  #define MEMFAULT_ENABLE_REBOOT_DIAG_DUMP 1
#endif

// Memfault SDK logs are routed to ESP-IDF logging, which are saved by Memfault,
// so it's redundant to save them in the Memfault SDK as well.
#define MEMFAULT_SDK_LOG_SAVE_DISABLE 1

// Kconfiglib will set this symbol to y even if SOC_CPU_CORES_NUM is not defined in older esp-idf
// versions. Instead, use the FreeRTOS-SMP configNUM_CORES to determine support
#if defined(CONFIG_MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT)
  #if defined(configNUM_CORES) && configNUM_CORES > 1
    #define MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT 1
  #endif
#endif

// Defaults to 1, so only disable if Kconfig setting is unset
#if !defined(CONFIG_MEMFAULT_METRICS_LOGS_ENABLE)
  #define MEMFAULT_METRICS_LOGS_ENABLE 0
#endif

// Pick up any user configuration overrides. This should be kept at the bottom
// of this file
#if CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL

  #if __has_include(CONFIG_MEMFAULT_PLATFORM_CONFIG_FILE)
    #include CONFIG_MEMFAULT_PLATFORM_CONFIG_FILE
  #endif

#else

  #include CONFIG_MEMFAULT_PLATFORM_CONFIG_FILE

#endif /* CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL */

#ifdef __cplusplus
}
#endif
