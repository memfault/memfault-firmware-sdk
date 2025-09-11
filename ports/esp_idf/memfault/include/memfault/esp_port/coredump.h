#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "esp_idf_version.h"
#include "memfault/panics/platform/coredump.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

//! MEMFAULT_ESP_PORT_NUM_REGIONS is the maximum number of regions that will be
//! filled by memfault_esp_port_coredump_get_regions().

//! Task WDT region is configurable, and only supported on esp-idf v5.3+
#if defined(CONFIG_MEMFAULT_COREDUMP_CAPTURE_TASK_WATCHDOG) && \
  (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4))
  #define MEMFAULT_ESP_PORT_TASK_WATCHDOG_REGION_COUNT 1
#else
  #define MEMFAULT_ESP_PORT_TASK_WATCHDOG_REGION_COUNT 0
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)
  // Memory regions used for esp-idf >= 4.4.3
  // Active stack (1) + task/timer and bss/sbss/common regions (6) + freertos tasks
  //(MEMFAULT_PLATFORM_MAX_TASK_REGIONS) + task_tcbs(1) + task_watermarks(1) +
  // bss(1) + data(1) + heap(1)
  #define MEMFAULT_ESP_PORT_NUM_REGIONS                               \
    (1 + 6 + MEMFAULT_PLATFORM_MAX_TASK_REGIONS + 1 + 1 + 1 + 1 + 1 + \
     MEMFAULT_ESP_PORT_TASK_WATCHDOG_REGION_COUNT)
#else  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)
  // Memory regions for esp-idf < 4.4.3
  // Active stack (1) + bss(1) + data(1) + heap(1)
  #define MEMFAULT_ESP_PORT_NUM_REGIONS \
    (1 + 1 + 1 + 1 + MEMFAULT_ESP_PORT_TASK_WATCHDOG_REGION_COUNT)
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)

//! ESP-IDF specific coredump region collection function.
//!
//! Returns an array of the regions to capture when the system crashes
//!
//! @param crash_info Information pertaining to the crash. The user of the SDK can decide
//!   whether or not to use this info when generating coredump regions to collect. Some
//!   example ideas can be found in the comments for the struct above
//! @param num_regions The number of regions in the list returned
const sMfltCoredumpRegion *memfault_esp_port_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions);

#ifdef __cplusplus
}
#endif
