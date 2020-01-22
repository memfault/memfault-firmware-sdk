#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Fake implementation of memfault storage

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void fake_memfault_platform_coredump_storage_setup(
    void *storage_buf, size_t storage_size, size_t sector_size);

bool fake_memfault_platform_coredump_storage_read(uint32_t offset, void *buf, size_t buf_len);
