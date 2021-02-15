//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "memfault/core/build_info.h"
#include "memfault_build_id_private.h"

#include <stdint.h>
#include <string.h>

#include "memfault/config.h"

#if MEMFAULT_USE_GNU_BUILD_ID

// Note: This variables is emitted by the linker script
extern uint8_t __start_gnu_build_id_start[];

MEMFAULT_BUILD_ID_QUALIFIER sMemfaultBuildIdStorage g_memfault_build_id = {
  .type = kMemfaultBuildIdType_GnuBuildIdSha1,
  .len = sizeof(sMemfaultElfNoteSection),
  .storage = __start_gnu_build_id_start,
};
#else

// NOTE: We start the array with a 0x1, so the compiler will never place the variable in bss
MEMFAULT_BUILD_ID_QUALIFIER uint8_t g_memfault_sdk_derived_build_id[MEMFAULT_BUILD_ID_LEN] = { 0x1, };

MEMFAULT_BUILD_ID_QUALIFIER sMemfaultBuildIdStorage g_memfault_build_id = {
  .type = kMemfaultBuildIdType_None,
  .len = sizeof(g_memfault_sdk_derived_build_id),
  .storage = g_memfault_sdk_derived_build_id,
};
#endif
