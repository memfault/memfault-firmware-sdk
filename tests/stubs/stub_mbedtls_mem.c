//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include <stdlib.h>

#include "mbedtls_mem.h"

void *__real_mbedtls_calloc(size_t n, size_t size) {
  return calloc(n, size);
}
void __real_mbedtls_free(void *ptr) {
  free(ptr);
}
