//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(mflt, LOG_LEVEL_DBG);

#include <string.h>

#include "memfault/components.h"

#include "memfault/ports/zephyr/http.h"

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = "<YOUR PROJECT KEY HERE>",
};

static char s_fw_version[16]="1.0.0+";

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  static bool s_init = false;

  if (!s_init) {
    const size_t version_len = strlen(s_fw_version);
    // We will use 6 characters of the build id to make our versions unique and
    // identifiable between releases
    const size_t build_id_chars = 6 + 1 /* '\0' */;

    const size_t build_id_num_chars =
        MEMFAULT_MIN(build_id_chars, sizeof(s_fw_version) - version_len - 1);

    memfault_build_id_get_string(&s_fw_version[version_len], build_id_num_chars);
    s_init = true;
  }

  *info = (sMemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-app",
    .software_version = s_fw_version,
    .hardware_version = CONFIG_BOARD,
  };
}

void main(void) {
  LOG_INF("Memfault Demo App! Board %s\n", CONFIG_BOARD);
  memfault_device_info_dump();
  memfault_zephyr_port_install_root_certs();
}
