#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void fake_memfault_event_storage_clear(void);
void fake_memfault_event_storage_set_available_space(size_t space_available);
void fake_event_storage_assert_contents_match(const void *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif
