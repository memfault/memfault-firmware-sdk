//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief
//! Example NRF52 specific routines for populating version data.

#include <stdio.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/device_info.h"
#include "nrf.h"

#ifndef MEMFAULT_NRF_MAIN_SOFTWARE_VERSION
#  define MEMFAULT_NRF_MAIN_SOFTWARE_VERSION "1.0.0"
#endif

#ifndef MEMFAULT_NRF_SOFTWARE_TYPE
#  define MEMFAULT_NRF_SOFTWARE_TYPE "nrf-main"
#endif

#ifndef MEMFAULT_NRF_HW_REVISION
#  define MEMFAULT_NRF_HW_REVISION "nrf-proto"
#endif

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
  prv_get_device_serial(s_device_serial, sizeof(s_device_serial));

  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = MEMFAULT_NRF_HW_REVISION,
    .software_version = MEMFAULT_NRF_MAIN_SOFTWARE_VERSION,
    .software_type = MEMFAULT_NRF_SOFTWARE_TYPE,
  };
}
