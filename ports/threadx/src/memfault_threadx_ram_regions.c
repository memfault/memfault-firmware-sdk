//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdint.h>
#include <string.h>

#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/ports/threadx_coredump.h"
#include "tx_api.h"

// ThreadX TCB validation ID - ASCII "THRD" (0x54485244). Defined in the
// internal tx_thread.h header; we replicate it here to avoid depending on that
// private header in a port file.
#ifndef TX_THREAD_ID
  #define TX_THREAD_ID ((ULONG)0x54485244UL)
#endif

// Sentinel for a thread whose stack is completely exhausted (0 free bytes at
// the bottom). Matches the FreeRTOS MEMFAULT_THREAD_STACK_0_BYTES_UNUSED
// convention so both can be decoded the same way by the backend.
#define MEMFAULT_THREADX_STACK_UNUSED_FULLY_EXHAUSTED UINT32_MAX

// One entry per tracked thread. Indexed in the same order as the
// created-thread list was walked when the coredump was captured.
typedef struct {
  uint32_t tx_thread_ptr;       // Address of the TX_THREAD control block
  uint32_t stack_bytes_unused;  // 0 = could not compute; UINT32_MAX = fully exhausted
} sMfltThreadXStackInfo;

static sMfltThreadXStackInfo s_mflt_threadx_stack_info[MEMFAULT_THREADX_MAX_THREADS];

// ThreadX global thread-list head and count, defined in tx_thread_initialize.c
extern TX_THREAD *_tx_thread_created_ptr;
extern ULONG _tx_thread_created_count;

// Scan the stack fill pattern from the bottom (stack_start, lowest address)
// upward and return the number of consecutive bytes still holding the fill
// value. Returns 0 if the sanitize check fails, or
// MEMFAULT_THREADX_STACK_UNUSED_FULLY_EXHAUSTED if the stack is fully used.
static uint32_t prv_scan_stack_bytes_unused(const TX_THREAD *thread) {
  // Validate the entire stack range before touching it
  const size_t sanitized = memfault_platform_sanitize_address_range(thread->tx_thread_stack_start,
                                                                    thread->tx_thread_stack_size);
  if (sanitized < thread->tx_thread_stack_size) {
    return 0;
  }

  // tx_thread_stack_fill_value exists only when both
  // TX_ENABLE_RANDOM_NUMBER_STACK_FILLING and TX_ENABLE_STACK_CHECKING are
  // defined. Otherwise ThreadX uses the fixed 0xEFEFEFEF pattern.
#if defined(TX_ENABLE_RANDOM_NUMBER_STACK_FILLING) && defined(TX_ENABLE_STACK_CHECKING)
  const ULONG fill = thread->tx_thread_stack_fill_value;
#else
  const ULONG fill = (ULONG)0xEFEFEFEFUL;
#endif

  const ULONG *stack_bottom = (const ULONG *)thread->tx_thread_stack_start;
  const ULONG stack_size_words = thread->tx_thread_stack_size / sizeof(ULONG);

  ULONG unused_words = 0;
  for (ULONG i = 0; i < stack_size_words; i++) {
    if (stack_bottom[i] != fill) {
      break;
    }
    unused_words++;
  }

  if (unused_words == 0) {
    return MEMFAULT_THREADX_STACK_UNUSED_FULLY_EXHAUSTED;
  }
  return (uint32_t)(unused_words * sizeof(ULONG));
}

size_t memfault_threadx_get_thread_regions(sMfltCoredumpRegion *regions, size_t num_regions) {
  if (regions == NULL || num_regions == 0) {
    return 0;
  }

  memset(s_mflt_threadx_stack_info, 0, sizeof(s_mflt_threadx_stack_info));

  size_t region_idx = 0;
  size_t info_idx = 0;

  TX_THREAD *t = _tx_thread_created_ptr;
  for (ULONG i = 0; i < _tx_thread_created_count && t != NULL; i++) {
    // Guard 1: TCB pointer must fall entirely within valid memory.
    if (memfault_platform_sanitize_address_range(t, sizeof(TX_THREAD)) < sizeof(TX_THREAD)) {
      break;
    }

    // Guard 2: TCB magic must be intact - if it isn't, the rest of the TCB
    // (including all stack-related fields) cannot be trusted.
    if (t->tx_thread_id != TX_THREAD_ID) {
      break;
    }

    // Capture the TCB.
    if (region_idx < num_regions) {
      regions[region_idx++] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(t, sizeof(TX_THREAD));
    }

    // Guard 3: stack region must be non-NULL and have a positive size.
    if (t->tx_thread_stack_start != NULL && t->tx_thread_stack_size > 0) {
      const size_t sanitized =
        memfault_platform_sanitize_address_range(t->tx_thread_stack_start, t->tx_thread_stack_size);

      if (sanitized > 0) {
        // Compute the watermark and record it in the sidecar.
        if (info_idx < MEMFAULT_ARRAY_SIZE(s_mflt_threadx_stack_info)) {
          s_mflt_threadx_stack_info[info_idx].tx_thread_ptr = (uint32_t)(uintptr_t)t;
          s_mflt_threadx_stack_info[info_idx].stack_bytes_unused = prv_scan_stack_bytes_unused(t);
          info_idx++;
        }

        // Capture the full stack region.
        if (region_idx < num_regions) {
          regions[region_idx++] =
            MEMFAULT_COREDUMP_MEMORY_REGION_INIT(t->tx_thread_stack_start, sanitized);
        }
      }
    }

    // Advance to next thread; detect circular-list wrap.
    TX_THREAD *next = t->tx_thread_created_next;
    if (next == _tx_thread_created_ptr) {
      break;
    }
    t = next;
  }

  // Append the watermark sidecar so the backend can read per-thread usage
  // without scanning fill patterns itself.
  if (info_idx > 0 && region_idx < num_regions) {
    regions[region_idx++] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&s_mflt_threadx_stack_info[0],
                                                                 sizeof(s_mflt_threadx_stack_info));
  }

  return region_idx;
}
