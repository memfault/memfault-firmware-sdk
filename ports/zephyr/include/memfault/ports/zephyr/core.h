#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Core Zephyr Memfault port module.

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/core/platform/device_info.h"

void memfault_zephyr_collect_reset_info(void);
void memfault_zephyr_get_device_info(sMemfaultDeviceInfo *info);

#ifdef __cplusplus
}
#endif
