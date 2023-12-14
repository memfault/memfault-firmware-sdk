//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake memfault_build_info_read implementation

#include "fake_memfault_build_id.h"

#include <string.h>

#define INITIAL_FAKE_BUILD_ID (sMemfaultBuildInfo) { \
  .build_id = { \
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, \
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, \
    0x01, 0x23, 0x45, 0x67, \
  }, \
}

eMemfaultBuildIdType g_fake_memfault_build_id_type = kMemfaultBuildIdType_None;
sMemfaultBuildInfo g_fake_memfault_build_id_info = INITIAL_FAKE_BUILD_ID;

void fake_memfault_build_id_reset(void) {
  g_fake_memfault_build_id_type = kMemfaultBuildIdType_None;
  g_fake_memfault_build_id_info = INITIAL_FAKE_BUILD_ID;
}

bool memfault_build_info_read(sMemfaultBuildInfo *info) {
  if (g_fake_memfault_build_id_type == kMemfaultBuildIdType_None) {
    return false;
  }
  memcpy(info->build_id, g_fake_memfault_build_id_info.build_id,
         sizeof(g_fake_memfault_build_id_info.build_id));
  return true;
}

bool memfault_build_id_get_string(char *out_buf, size_t buf_len) {
  if (g_fake_memfault_build_id_type == kMemfaultBuildIdType_None) {
    return false;
  }
  const char *fake_build_id = "0123456789abcdef0123456789abcdef01234567";
  strncpy(out_buf, fake_build_id, buf_len);
  return true;
}
