//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief

#include "memfault/core/build_info.h"
#include "memfault_build_id_private.h"

#include <string.h>
#include <stdio.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"

static const void *prv_get_build_id_start_pointer(void) {
  switch (g_memfault_build_id.type) {
    case kMemfaultBuildIdType_MemfaultBuildIdSha1:
      return g_memfault_build_id.storage;
    case kMemfaultBuildIdType_GnuBuildIdSha1: {
      const sMemfaultElfNoteSection *elf =
          (const sMemfaultElfNoteSection *)g_memfault_build_id.storage;
      return &elf->namedata[elf->namesz]; // Skip over { 'G', 'N', 'U', '\0' }
    }
    case kMemfaultBuildIdType_None:
    default:
      break;
  }

  return NULL;
}

bool memfault_build_info_read(sMemfaultBuildInfo *info) {
  const void *id = prv_get_build_id_start_pointer();
  if (id == NULL) {
    return false;
  }

  memcpy(info->build_id, id, sizeof(info->build_id));
  return true;
}

static char prv_nib_to_hex_ascii(uint8_t val) {
  return val < 10 ? (char)val + '0' : (char)(val - 10) + 'a';
}

void memfault_build_info_dump(void) {
  sMemfaultBuildInfo info;
  bool id_available = memfault_build_info_read(&info);
  if (!id_available) {
    MEMFAULT_LOG_INFO("No Build ID available");
    return;
  }

  const bool is_gnu =
      (g_memfault_build_id.type == kMemfaultBuildIdType_GnuBuildIdSha1);

  char build_id_sha[41] = { 0 };
  for (size_t i = 0; i < sizeof(info.build_id); i++) {
    uint8_t c = info.build_id[i];
    size_t idx = i * 2;
    build_id_sha[idx] = prv_nib_to_hex_ascii((c >> 4) & 0xf);
    build_id_sha[idx + 1] = prv_nib_to_hex_ascii(c & 0xf);
  }

  MEMFAULT_LOG_INFO("%s Build ID: %s", is_gnu ? "GNU" : "Memfault", build_id_sha);
}
