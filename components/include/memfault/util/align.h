//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/sdk_assert.h"

// C99 does not have a max_align_t type, so we use a union
// of types with the largest alignments to determine max align
// Taken from
// https://github.com/zephyrproject-rtos/zephyr/blob/bfa0a87277c31d9e1b36104a29b17f3fc1a47444/include/zephyr/types.h#L22-L30
typedef union {
  long long thelonglong;
  long double thelongdouble;
  uintmax_t theuintmax_t;
  size_t thesize_t;
  uintptr_t theuintptr_t;
  void *thepvoid;
  void (*thepfunc)(void);
} uMemfaultMaxAlignType;

#if !defined(MEMFAULT_ALIGNOF)
  #error \
    "Compiler has no __alignof__ or equivalent extension. Please disable MEMFAULT_MBEDTLS_METRICS or contact support"
#endif  // !defined(MEMFAULT_ALIGN)

// Allow override for tests
#if !defined(MEMFAULT_MAX_ALIGN_SIZE)
  #define MEMFAULT_MAX_ALIGN_SIZE (MEMFAULT_ALIGNOF(uMemfaultMaxAlignType))
#endif  // !defined(MEMFAULT_MAX_ALIGN_SIZE)

#ifdef __cplusplus
}
#endif
