#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! Fake implementation of memfault storage

#include <stddef.h>

void fake_memfault_platform_coredump_storage_setup(
    void *storage_buf, size_t storage_size, size_t sector_size);
