//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! An example implementation of the CRC platform APIs for the WICED SDK

#include "memfault/core/platform/crc32.h"

#include "crc.h"

uint32_t memfault_platform_crc32(const void *data, size_t data_len) {
  if (data == NULL) {
    return 0;
  }

  return WICED_CRC_FUNCTION_NAME(crc32)((void *)data, data_len, CRC32_INIT_VALUE);
}
