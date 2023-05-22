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

//! Define base number of regions to collect
#define MEMFAULT_ZEPHYR_BASE_COREDUMP_REGIONS                       \
  (/* active stack(s) [2] */                                        \
   (IS_ENABLED(CONFIG_MEMFAULT_COREDUMP_COLLECT_STACK_REGIONS) * 2) \
                                                                    \
   /* _kernel [1] */                                                \
   + IS_ENABLED(CONFIG_MEMFAULT_COREDUMP_COLLECT_KERNEL_REGION)     \
                                                                    \
   /* s_task_watermarks + s_task_tcbs [2] */                        \
   + (IS_ENABLED(CONFIG_MEMFAULT_COREDUMP_COLLECT_TASKS_REGIONS) * 2))

//! Define the total regions to collect
#define MEMFAULT_ZEPHYR_COREDUMP_REGIONS                                        \
  (MEMFAULT_ZEPHYR_BASE_COREDUMP_REGIONS + MEMFAULT_COREDUMP_MAX_TASK_REGIONS + \
   IS_ENABLED(CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS) +                   \
   IS_ENABLED(CONFIG_MEMFAULT_COREDUMP_COLLECT_DATA_REGIONS))

//! Helper to collect regions required to decode Zephyr state
//!
//! These regions include the active task, the kernel state, other tasks in the system,
//! and bss/data regions if enabled.
//!
//! @param crash_info Contains info regarding the location and state of the crash
//! @param regions Pointer to save region info into
//! @param num_regions The number of entries in the 'regions' array
size_t memfault_zephyr_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                            sMfltCoredumpRegion *regions, size_t num_regions);

//! Helper to collect minimal RAM needed for backtraces of non-running Zephyr tasks
//!
//! @param[out] regions Populated with the regions that need to be collected in order
//!  for task and stack state to be recovered for non-running Zephyr tasks
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
