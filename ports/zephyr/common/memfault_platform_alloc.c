//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include "memfault/ports/zephyr/alloc.h"

// clang-format off
#include <stdlib.h>

#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
// clang-format on

// When RAM is already reserved for the libc heap, we prefer it over the Zephyr kernel heap
// to avoid a separate RAM allocation.
//
// CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE was added in Zephyr v3.4.0; guard with defined() so
// -Werror=undef doesn't trip on older Zephyr versions where the symbol doesn't exist.
#if (defined(CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE) && \
     CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE != 0) ||    \
  (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE != 0)
void *memfault_zephyr_port_calloc(size_t count, size_t size) {
  return calloc(count, size);
}

void memfault_zephyr_port_free(void *ptr) {
  free(ptr);
}
#elif CONFIG_HEAP_MEM_POOL_SIZE > 0
void *memfault_zephyr_port_calloc(size_t count, size_t size) {
  return k_calloc(count, size);
}

void memfault_zephyr_port_free(void *ptr) {
  k_free(ptr);
}
#else
  #error \
    "One of CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE, CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE, or CONFIG_HEAP_MEM_POOL_SIZE must be non-zero"
#endif
