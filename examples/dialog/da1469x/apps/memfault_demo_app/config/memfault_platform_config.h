#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH    1

// Note: The default location coredumps are saved is NVMS log storage.
// This size can be adjusted depending on the amount of RAM regions collected
// in memfault_platform_coredump_get_regions()
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_MAX_SIZE_BYTES (32 * 1024)

#define MEMFAULT_USE_GNU_BUILD_ID 1

#ifdef __cplusplus
}
#endif
