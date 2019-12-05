//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details

#include <zephyr.h>
#include <misc/printk.h>

// Product-specific Memfault dependencies
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-main",
    .software_version = "1.0.0",
    .hardware_version = "disco_l475_iot1",
  };
}

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = "<YOUR API KEY HERE>",
};

void main(void) {
  printk("Memfault Demo App! Board %s\n", CONFIG_BOARD);
}
