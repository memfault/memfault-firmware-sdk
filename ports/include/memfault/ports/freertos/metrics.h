#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief

#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #if !defined(CONFIG_IDF_TARGET_ESP8266)
    #define MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #endif
#endif

#include "memfault/config.h"

#ifdef MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#else
  #include "FreeRTOS.h"
  #include "task.h"
#endif

// The FreeRTOS task.h header provides these version specifiers starting with
// FreeRTOS V8.0.0.
#define MEMFAULT_FREERTOS_VERSION_GTE(major, minor, build)                   \
  (tskKERNEL_VERSION_MAJOR > major) ||                                       \
    (tskKERNEL_VERSION_MAJOR == major && tskKERNEL_VERSION_MINOR > minor) || \
    (tskKERNEL_VERSION_MAJOR == major && tskKERNEL_VERSION_MINOR == minor && \
     tskKERNEL_VERSION_BUILD >= build)

// Auto enable run time stats if the FreeRTOS version is >= 10.2.0 and and the
// FreeRTOS config options are set appropriately
#if configGENERATE_RUN_TIME_STATS && configUSE_TRACE_FACILITY && \
  MEMFAULT_FREERTOS_VERSION_GTE(10, 2, 0)
  // For now only support ESP-IDF multi-core FreeRTOS until the
  // FreeRTOS kernel mainlines support for SMP.
  #if defined(ESP_PLATFORM) && configNUM_CORES == 2
    #define MEMFAULT_FREERTOS_RUN_TIME_STATS_SINGLE_CORE 0
    #define MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE 1
  #else
    #define MEMFAULT_FREERTOS_RUN_TIME_STATS_SINGLE_CORE 1
    #define MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE 0
  #endif
#else
  // Not available
  #define MEMFAULT_FREERTOS_RUN_TIME_STATS_SINGLE_CORE 0
  #define MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE 0
#endif

// The user can opt-out by setting MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS to 0
// in memfault_platform_config.h
#if !defined(MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS)
  #define MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS \
    (MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE || MEMFAULT_FREERTOS_RUN_TIME_STATS_SINGLE_CORE)
#endif

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void memfault_freertos_port_task_runtime_metrics(void);

#ifdef __cplusplus
}
#endif  // __cplusplus
