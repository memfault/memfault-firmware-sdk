//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example populating memfault_platform_get_device_info() implementation for DA1469x

#include <stdio.h>
#include <string.h>

#include "memfault/components.h"

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (struct MemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_version = memfault_create_unique_version_string("1.0.0"),
    .hardware_version = "DA14695",
    .software_type = "da1469x-demo-app",
  };
}
