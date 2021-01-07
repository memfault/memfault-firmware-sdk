//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_STORAGE_SIZE (256)
extern uint8_t g_ram_backed_storage[MEMFAULT_STORAGE_SIZE];

void fake_buffered_coredump_storage_reset(void);
void fake_buffered_coredump_storage_set_size(size_t size);
void fake_buffered_coredump_inject_write_failure(void);

#ifdef __cplusplus
}
#endif
