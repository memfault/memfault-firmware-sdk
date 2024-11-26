#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Implements thread tracking for ThreadX. This file should be included in the
//! ThreadX port layer and the following defines should be added to the ThreadX
//! configuration file:
//!
// clang-format off
//!
//! #define TX_THREAD_CREATE_EXTENSION(thread_ptr) memfault_threadx_thread_create_extension(thread_ptr)
//! #define TX_THREAD_DELETE_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
//! #define TX_THREAD_COMPLETED_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
//! #define TX_THREAD_TERMINATED_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
// clang-format on

#include <stddef.h>

#include "memfault/config.h"
#include "memfault/panics/platform/coredump.h"
#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

//! For each thread tracked we will need to collect the TCB + a region of the stack
#define MEMFAULT_PLATFORM_MAX_TASK_REGIONS \
  (MEMFAULT_PLATFORM_MAX_TRACKED_TASKS * (1 /* TCB */ + 1 /* stack */))

//! Helper to collect minimal RAM needed for backtraces of non-running ThreadX threads
//!
//! @param[out] regions Populated with the regions that need to be collected in order
//!  for task and stack state to be recovered for non-running ThreadX tasks
//! @param[in] num_regions The number of entries in the 'regions' array
//!
//! @return The number of entries that were populated in the 'regions' argument. Will always
//!  be <= num_regions
size_t memfault_threadx_get_thread_regions(sMfltCoredumpRegion *regions, size_t num_regions);

// clang-format off
//! ThreadX thread creation/deletion hooks. These functions are called by the
//! ThreadX kernel when a thread is created or deleted. They are used to track
//! the TCBs and stacks of all threads in the system.
//!
//! NOTE: these must be installed by adding the following lines to the platform
//! tx_port.h configuration file:
//! #define TX_THREAD_CREATE_EXTENSION(thread_ptr) memfault_threadx_thread_create_extension(thread_ptr)
//! #define TX_THREAD_DELETE_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
//! #define TX_THREAD_COMPLETED_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
//! #define TX_THREAD_TERMINATED_EXTENSION(thread_ptr) memfault_threadx_thread_delete_extension(thread_ptr)
//!
//! @param[in] thread_ptr The ThreadX thread control block (TCB) of the thread
// clang-format on
void memfault_threadx_thread_create_extension(const TX_THREAD *thread_ptr);
void memfault_threadx_thread_delete_extension(const TX_THREAD *thread_ptr);

#ifdef __cplusplus
}
#endif
