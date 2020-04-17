#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/core/platform/system_time.h"

#ifdef __cplusplus
extern "C" {
#endif

void fake_memfault_platform_time_enable(bool enable);
void fake_memfault_platform_time_set(const sMemfaultCurrentTime *time);

#ifdef __cplusplus
}
#endif
