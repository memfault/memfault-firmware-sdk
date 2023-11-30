//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include <stddef.h>

#include "memfault/core/math.h"
#include "memfault/metrics/metrics.h"
#include "memfault/ports/mbedtls/metrics.h"
#include "memfault/util/align.h"

// We create a metadata type who's size is equal to the maximum alignment of the architecture
// This allows us to safely prefix the allocated memory with our metadata and return an aligned
// offset value to the caller
typedef union {
  uint32_t requested_size;
  uMemfaultMaxAlignType padding;
} uAllocMetadata;

#define ALLOC_METADATA_OVERHEAD (sizeof(uAllocMetadata))

// Due to the metadata structure, we require the largest datatype is >= than the largest alignment
// requirement
MEMFAULT_STATIC_ASSERT(ALLOC_METADATA_OVERHEAD >= MEMFAULT_MAX_ALIGN_SIZE,
                       "sizeof(uAllocMetadata) must be >= to maximum alignment size");

// Additionally we require that the sizeof our metadata structure be a multiple of the alignment
MEMFAULT_STATIC_ASSERT(ALLOC_METADATA_OVERHEAD % MEMFAULT_MAX_ALIGN_SIZE == 0,
                       "uAllocMetadata must be a multiple of the maximum alignment");

extern void *__real_mbedtls_calloc(size_t n, size_t size);
extern void __real_mbedtls_free(void *ptr);

static int32_t s_mbedtls_mem_used = 0;
static uint32_t s_mbedtls_mem_max = 0;

// This wrapper adds allocations with a metadata structure at the beginning of the allocation.
// The memory allocated is large enough to account for overhead of the metadata structure,
// including its padding. The metadata structure must be equal in size to the maximum
// alignment. This is because a fixed offset is used to transform the pointer from the wrapped
// pointer containing the metadata structure and the pointer returned to the caller.
void *__wrap_mbedtls_calloc(size_t n, size_t size) {
  // Pointer to return to the caller
  void *aligned_ptr = NULL;
  // Requested allocation size
  size_t requested_size = n * size;
  // Size required by maximum alignment (includes stat and alignment padding)
  size_t total_size = requested_size + ALLOC_METADATA_OVERHEAD;

  void *wrapper_ptr = __real_mbedtls_calloc(1, total_size);
  if (wrapper_ptr) {
    // Offset wrapped pointer by metadata overhead and round up to next aligned multiple
    aligned_ptr = (void *)((uintptr_t)wrapper_ptr + ALLOC_METADATA_OVERHEAD);

    // Get metadata pointer and update the fields
    uAllocMetadata *metadata_ptr = (uAllocMetadata *)wrapper_ptr;
    metadata_ptr->requested_size = requested_size;

    // Update metric values
    s_mbedtls_mem_used += requested_size;
    if (s_mbedtls_mem_used > (int32_t)s_mbedtls_mem_max) {
      s_mbedtls_mem_max = (uint32_t)s_mbedtls_mem_used;
    }
  }

  return aligned_ptr;
}

// This wrapper performs the opposite operations of the calloc wrapper. The memory allocation
// is arranged such that we can safely subtract the provided pointer by the size of the
// metadata structure to access the metadata stored to update the memory metrics and restore
// the originally allocated pointer.
void __wrap_mbedtls_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  // Get pointer to stat structure
  uAllocMetadata *metadata_ptr = (uAllocMetadata *)ptr - 1;
  // Determine original pointer from stat offset
  void *orig_ptr = (void *)metadata_ptr;

  // Update metric
  s_mbedtls_mem_used -= metadata_ptr->requested_size;
  // Free original pointer
  __real_mbedtls_free(orig_ptr);
}

void memfault_mbedtls_heartbeat_collect_data(void) {
  MEMFAULT_METRIC_SET_SIGNED(mbedtls_mem_used_bytes, s_mbedtls_mem_used);
  MEMFAULT_METRIC_SET_UNSIGNED(mbedtls_mem_max_bytes, s_mbedtls_mem_max);
}

void memfault_mbedtls_test_clear_values(void) {
  s_mbedtls_mem_used = 0;
  s_mbedtls_mem_max = 0;
}
