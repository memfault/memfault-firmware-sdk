//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! @brief
//! Example NRF52 specific routines for populating version data.

#include <stdio.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/device_info.h"
#include "nrf.h"

#ifndef MEMFAULT_NRF_MAIN_FIRMWARE_VERSION
#  define MEMFAULT_NRF_MAIN_FIRMWARE_VERSION "1.0.0"
#endif

#ifndef MEMFAULT_NRF_HW_REVISION
#  define MEMFAULT_NRF_HW_REVISION "nrf-proto"
#endif

void memfault_platform_get_unique_device_id(char *buf, size_t buf_len) {
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
  static char s_fw_version[32];
  strncpy(s_fw_version, MEMFAULT_NRF_MAIN_FIRMWARE_VERSION, sizeof(s_fw_version));

  static char s_device_serial[MEMFAULT_MAX_SERIAL_NUMBER_LEN];
  memfault_platform_get_unique_device_id(s_device_serial, sizeof(s_device_serial));

  static char s_hw_rev[MEMFAULT_MAX_HW_REVISION_LEN];
  strncpy(s_hw_rev, MEMFAULT_NRF_HW_REVISION, sizeof(s_hw_rev));

  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = s_hw_rev,
    .fw_version = s_fw_version,
  };
}
