#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!

#include <stddef.h>

#include "memfault/panics/platform/coredump.h"

#ifdef __cplusplus
extern "C" {
#endif

//! For each task, we will collect the TCB and the portion of the stack where context is saved
#define MEMFAULT_COREDUMP_MAX_TASK_REGIONS (CONFIG_MEMFAULT_COREDUMP_MAX_TRACKED_TASKS * 2)

//! Given a pointer and size returns the actual size which should be collected.
//!
//! @note This is used to make sure the memory being collected is within the bounds
//! of the MCUs memory map.
//! @note A user _must_ implement this function if they are using
//! "memfault_freertos_get_task_regions()"
//!
//! @param[in] start_addr The address of the start of the memory range to collect
//! @param[in] desired_size The size trying to be collected for the region
//!
//! @return The actual size to collect or 0 if the address is invalid.
extern size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size);


//! Helper to collect minimal RAM needed for backtraces of non-running FreeRTOS tasks
//!
//! @param[out] regions Populated with the regions that need to be collected in order
//!  for task and stack state to be recovered for non-running FreeRTOS tasks
//! @param[in] num_regions The number of entries in the 'regions' array
//!
//! @return The number of entries that were populated in the 'regions' argument. Will always
//!  be <= num_regions
size_t memfault_zephyr_get_task_regions(sMfltCoredumpRegion *regions, size_t num_regions);

//! Helper to collect regions of RAM used for BSS variables
//!
//! @return The number of entries that were populated in the 'regions' argument. Will always
//!  be <= num_regions
size_t memfault_zephyr_get_bss_regions(sMfltCoredumpRegion *regions, size_t num_regions);

//! Helper to collect regions of RAM used for DATA variables
//!
//! @return The number of entries that were populated in the 'regions' argument. Will always
//!  be <= num_regions
size_t memfault_zephyr_get_data_regions(sMfltCoredumpRegion *regions, size_t num_regions);

#ifdef __cplusplus
}
#endif
