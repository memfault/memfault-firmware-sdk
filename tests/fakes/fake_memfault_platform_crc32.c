//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Simple example implementation of standard Zip/Ethernet CRC polynomial. In real life, you
//! probably want to use a lookup table to speed up the computation

#include "memfault/core/platform/crc32.h"

static uint32_t prv_crc32_for_byte(uint32_t b) {
  for (size_t i = 0; i < 8; i++) {
    b = (((b & 0x1) != 0) ? 0 : (uint32_t)0xEDB88320L) ^ b >> 1;
  }
  return b ^ (uint32_t)0xFF000000L;
}


uint32_t memfault_platform_crc32(const void *data, size_t data_len) {
  const uint8_t *byte = data;

  uint32_t crc = 0;
  for (size_t i = 0; i < data_len; i++) {
    uint32_t idx = ((crc & 0xff) ^ byte[i]);
    crc = prv_crc32_for_byte(idx) ^ crc >> 8;
  }

  return crc;
}
