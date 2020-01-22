//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief
//! Example MBed specific routines for populating version data.
#include "memfault/core/platform/device_info.h"

#include <stdio.h>
#include <string.h>

#include "memfault/core/math.h"

#ifndef MEMFAULT_MBED_MAIN_SOFTWARE_VERSION
#  define MEMFAULT_MBED_MAIN_SOFTWARE_VERSION "1.0.0"
#endif

#ifndef MEMFAULT_MBED_SOFTWARE_TYPE
#  define MEMFAULT_MBED_SOFTWARE_TYPE "mbed-main"
#endif

#ifndef MEMFAULT_MBED_HW_REVISION
#  define MEMFAULT_MBED_HW_REVISION "mbed-proto"
#endif

#ifdef TARGET_STM32F4
#include "stm32f4xx_hal.h"

static char prv_nib_to_hex_ascii(uint8_t val) {
  return val < 10 ? (char)val + '0' : (char)(val - 10) + 'A';
}

#define MEMFAULT_MBED_UID_SIZE_IN_WORDS (3)
#define MEMFAULT_MBED_UID_SIZE_IN_BYTES (MEMFAULT_MBED_UID_SIZE_IN_WORDS * 4)
#define MEMFAULT_MBED_UID_SIZE_IN_ASCII (MEMFAULT_MBED_UID_SIZE_IN_BYTES * 2)

static char *prv_get_device_serial(void) {
  uint32_t uid[MEMFAULT_MBED_UID_SIZE_IN_WORDS];
  HAL_GetUID(uid);
  uint8_t *byte_reader = (uint8_t *)uid;

  static char s_device_serial[MEMFAULT_MBED_UID_SIZE_IN_ASCII + 1]; // +1 for null
  char *serial_write_ptr = &s_device_serial[0];

  for (size_t i=0; i<MEMFAULT_MBED_UID_SIZE_IN_BYTES; i++) {
    uint8_t c = byte_reader[i];
    *serial_write_ptr++ = prv_nib_to_hex_ascii(c >> 4);
    *serial_write_ptr++ = prv_nib_to_hex_ascii(c & 0xf);
  }
  *serial_write_ptr = 0; // null terminate the ascii string
  return s_device_serial;
}
#else
/*
 * TODO: At the time this example was created (MBed OS 5.14), no release version
 * had support for reading a target's unique id in a platform independent way
 * but they were working on it.  This example should use it when it is released.
 * See: https://github.com/ARMmbed/mbed-os/pull/5557
 *
 * This example supports reading the unique ID from the STM32F4xx only.  We only
 * use the unique ID as a fake device serial number for demo purposes.  You'll
 * need to implement prv_get_device_serial() here for your target.  To just see
 * the demo run, you could even use a dummy string.
 */
#error Querying unique ID is only supported for STM32F4; you need to implement prv_get_device_serial()
#endif

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  *info = (struct MemfaultDeviceInfo) {
    .device_serial = prv_get_device_serial(),
    .hardware_version = MEMFAULT_MBED_HW_REVISION,
    .software_version = MEMFAULT_MBED_MAIN_SOFTWARE_VERSION,
    .software_type = MEMFAULT_MBED_SOFTWARE_TYPE,
  };
}
