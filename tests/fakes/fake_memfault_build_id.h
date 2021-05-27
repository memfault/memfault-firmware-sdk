#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stddef.h>

#include "memfault/core/build_info.h"
#include "memfault_build_id_private.h"

#ifdef __cplusplus
extern "C" {
#endif

extern eMemfaultBuildIdType g_fake_memfault_build_id_type;
extern sMemfaultBuildInfo g_fake_memfault_build_id_info;

void fake_memfault_build_id_reset(void);

#ifdef __cplusplus
}
#endif
