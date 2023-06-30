//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Declarations for mbedtls memory functions
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
void *__wrap_mbedtls_calloc(size_t n, size_t size);
void __wrap_mbedtls_free(void *ptr);
void *__real_mbedtls_calloc(size_t n, size_t size);
void __real_mbedtls_free(void *ptr);
#ifdef __cplusplus
}
#endif
