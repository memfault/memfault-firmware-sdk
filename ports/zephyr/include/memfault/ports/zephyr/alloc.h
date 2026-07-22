//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Shared calloc()/free() wrappers for Zephyr ports that need heap allocation
//! (e.g. HTTP and CoAP transports), backed by whichever heap the platform has
//! configured. Implemented in memfault_platform_alloc.c.

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memfault_zephyr_port_calloc(size_t count, size_t size);
void memfault_zephyr_port_free(void *ptr);

#ifdef __cplusplus
}
#endif
