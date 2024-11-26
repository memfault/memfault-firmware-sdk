//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Implements thread tracking for ThreadX.

#include "memfault/components.h"
#include "memfault/ports/threadx/threadx.h"
#include "tx_api.h"

//! Track the TCB pointer, stack location, and stack size for each thread. This
//! is recorded on thread creation, in case the TCB is corrupted before coredump
//! recording is executed; this gives us a better chance of capturing the actual
//! stack region for each thread (vs. using the information in the TCB to locate
//! the stack regions at coredump time) at the expense of +8 bytes per thread.
static struct MemfaultThreadXThreadInfo {
  const TX_THREAD *thread_ptr;
  const void *stack_ptr;
  size_t stack_size;
} s_memfault_threadx_threads[MEMFAULT_PLATFORM_MAX_TASK_REGIONS];

void memfault_threadx_thread_create_extension(const TX_THREAD *thread_ptr) {
  // Find an empty slot in the thread tracking array and record the TCB and stack information`
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_memfault_threadx_threads); i++) {
    if (s_memfault_threadx_threads[i].thread_ptr == NULL) {
      s_memfault_threadx_threads[i].thread_ptr = thread_ptr;
      s_memfault_threadx_threads[i].stack_ptr = thread_ptr->tx_thread_stack_ptr;
      s_memfault_threadx_threads[i].stack_size = thread_ptr->tx_thread_stack_size;
      break;
    }
  }
}

void memfault_threadx_thread_delete_extension(const TX_THREAD *thread_ptr) {
  // Clear the thread tracking array entry for the deleted thread
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_memfault_threadx_threads); i++) {
    if (s_memfault_threadx_threads[i].thread_ptr == thread_ptr) {
      s_memfault_threadx_threads[i].thread_ptr = NULL;
      break;
    }
  }
}

size_t memfault_threadx_get_thread_regions(sMfltCoredumpRegion *regions, size_t num_regions) {
  size_t region_idx = 0;

  // Add a region for each TCB and stack in the thread tracking array
  for (size_t i = 0;
       i < MEMFAULT_ARRAY_SIZE(s_memfault_threadx_threads) && region_idx + 1 < num_regions; i++) {
    if (s_memfault_threadx_threads[i].thread_ptr == NULL) {
      continue;
    }

    regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      s_memfault_threadx_threads[i].thread_ptr, sizeof(TX_THREAD));
    region_idx++;

    regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      s_memfault_threadx_threads[i].stack_ptr, s_memfault_threadx_threads[i].stack_size);
    region_idx++;
  }

  return region_idx;
}
