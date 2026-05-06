#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! ThreadX coredump helper: capture per-thread TCBs, stacks, and a stack-usage
//! watermark sidecar into coredump regions.
//!
//! Usage from memfault_platform_coredump_get_regions():
//!
//!   #include "memfault/ports/threadx_coredump.h"
//!
//!   // ... after capturing active stack and kernel BSS ...
//!   region_idx += memfault_threadx_get_thread_regions(
//!     &s_coredump_regions[region_idx],
//!     MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

#include <stddef.h>

#include "memfault/config.h"
#include "memfault/panics/platform/coredump.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Maximum number of ThreadX threads whose TCB + stack + watermark are captured.
//! Override in memfault_platform_config.h if the application has more threads.
#ifndef MEMFAULT_THREADX_MAX_THREADS
  #define MEMFAULT_THREADX_MAX_THREADS 16
#endif

//! Upper bound on regions returned by memfault_threadx_get_thread_regions():
//!   MEMFAULT_THREADX_MAX_THREADS TCB regions
//!   MEMFAULT_THREADX_MAX_THREADS stack regions
//!   1 watermark sidecar region
#define MEMFAULT_THREADX_MAX_TASK_REGIONS (MEMFAULT_THREADX_MAX_THREADS * 2 + 1)

//! Walk the ThreadX created-thread circular list and populate @p regions with:
//!   - Each thread's TX_THREAD control block
//!   - Each thread's full stack (validated via memfault_platform_sanitize_address_range)
//!   - A stack-usage watermark sidecar array (one entry per thread)
//!
//! The walk is safe against corrupted TCBs: each pointer is sanitized and each
//! TCB's tx_thread_id field is checked before any stack fields are accessed.
//!
//! @param regions    Output array to populate with coredump regions.
//! @param num_regions Size of the @p regions array.
//! @return Number of entries written to @p regions (always <= @p num_regions).
size_t memfault_threadx_get_thread_regions(sMfltCoredumpRegion *regions, size_t num_regions);

#ifdef __cplusplus
}
#endif
