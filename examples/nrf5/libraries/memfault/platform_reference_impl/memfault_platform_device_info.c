//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief
//! Example NRF52 specific routines for populating version data.

#include <stdio.h>
#include <string.h>

#include "memfault/components.h"
#include "nrf.h"

static void prv_get_device_serial(char *buf, size_t buf_len) {
  // We will use the 64bit NRF "Device identifier" as the serial number
  const size_t nrf_uid_num_words = 2;

  size_t curr_idx = 0;
  for (size_t i = 0; i < nrf_uid_num_words; i++) {
    uint32_t lsw = NRF_FICR->DEVICEID[i];

    const size_t bytes_per_word =  4;
    for (size_t j = 0; j < bytes_per_word; j++) {

      size_t space_left = buf_len - curr_idx;
      uint8_t val = (lsw >> (j * 8)) & 0xff;
      size_t bytes_written = snprintf(&buf[curr_idx], space_left, "%02X", (int)val);
      if (bytes_written < space_left) {
        curr_idx += bytes_written;
      } else { // we are out of space, return what we got, it's NULL terminated
        return;
      }
    }
  }
}

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  static char s_device_serial[32];
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
    .hardware_version = "pca10056",
    .software_version = s_fw_version,
    .software_type = "nrf-main",
  };
}
