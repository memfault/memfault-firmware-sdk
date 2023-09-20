//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include <string.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/http.h"
// clang-format on

LOG_MODULE_REGISTER(mflt_app, LOG_LEVEL_DBG);

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = "<YOUR PROJECT KEY HERE>",
};

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-app",
    .software_version = "1.0.0-dev",
    .hardware_version = CONFIG_BOARD,
  };
}

int main(void) {
  LOG_INF("Memfault Demo App! Board %s\n", CONFIG_BOARD);
  memfault_device_info_dump();
  memfault_zephyr_port_install_root_certs();

  return 0;
}
