//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <zephyr.h>
#include <sys/printk.h>

#include "memfault/core/platform/device_info.h"

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  // platform specific version information
  *info = (sMemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "main-fw",
    .software_version = "1.0.0-dev1",
    .hardware_version = "proto-hardware",
  };
}

void main(void) {
  printk("Hello World! %s\n", CONFIG_BOARD);
}
