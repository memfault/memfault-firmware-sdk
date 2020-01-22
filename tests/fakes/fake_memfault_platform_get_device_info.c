//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of the memfault_platform_get_device_info platform API

#include "fake_memfault_platform_get_device_info.h"

#include "memfault/core/platform/device_info.h"

struct MemfaultDeviceInfo g_fake_device_info = {
    .device_serial = "DAABBCCDD",
    .software_version = "1.2.3",
    .software_type = "main",
    .hardware_version = "evt_24",
};

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  *info = g_fake_device_info;
}
