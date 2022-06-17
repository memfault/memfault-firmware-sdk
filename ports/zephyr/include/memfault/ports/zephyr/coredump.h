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

//! Run the Zephyr z_fatal_error function. This is used to execute the Zephyr
//! error console prints, which are suppressed due to the Memfault fault handler
//! replacing the z_fatal_error function at link time.
//!
//! This can be useful when locally debugging without a debug probe connected.
//! It's called as part of the built-in implementation of
//! memfault_platform_reboot(); if a user-implemented version of that function
//! is used, this function can be called from there.
void memfault_zephyr_z_fatal_error(void);

#ifdef __cplusplus
}
#endif
