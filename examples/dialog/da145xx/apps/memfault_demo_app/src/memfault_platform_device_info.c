//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example populating memfault_platform_get_device_info() implementation for DA145xx

#include <stdio.h>

#include "memfault/components.h"
#include "nvds.h"

static void prv_get_device_serial(char *buf, size_t buf_len) {
  nvds_tag_len_t tag_len;
  struct bd_addr dev_bdaddr;
  if (nvds_get(NVDS_TAG_BD_ADDRESS, &tag_len, (uint8_t *)&dev_bdaddr) == NVDS_OK) {
    (void)snprintf(buf, buf_len, "%02X%02X%02X%02X%02X%02X",
        dev_bdaddr.addr[5], dev_bdaddr.addr[4], dev_bdaddr.addr[3],
        dev_bdaddr.addr[2], dev_bdaddr.addr[1], dev_bdaddr.addr[0]);
  }
}

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  static char s_device_serial[12 + 1]; /* \0 */
  static char s_fw_version[16] = "1.0.0+";
  static bool s_init = false;

  if (!s_init) {
    prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
    const size_t version_len = strlen(s_fw_version);
    // We will use 6 characters of the build id to make our versions unique and
    // identifiable between releases
    const size_t build_id_chars = 6 + 1 /* '\0' */;
    const size_t build_id_num_chars =
        MEMFAULT_MIN(build_id_chars, sizeof(s_fw_version) - version_len - 1);

    memfault_build_id_get_string(&s_fw_version[version_len], build_id_num_chars);
    s_init = true;
  }

  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = APP_DIS_HARD_REV_STR,
    .software_version = s_fw_version,
#if defined (__DA14531__)
    .software_type = "da14531-demo-app",
#elif defined (__DA14586__)
    .software_type = "da14586-demo-app",
#else
    .software_type = "da14585-demo-app",
#endif
  };
}
