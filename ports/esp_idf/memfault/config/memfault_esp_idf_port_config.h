#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
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

#if defined(CONFIG_MEMFAULT_SYNC_MEMFAULT_METRICS)
  #define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 1
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
