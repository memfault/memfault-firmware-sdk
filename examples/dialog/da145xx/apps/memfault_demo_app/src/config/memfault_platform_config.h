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

// Save coredumps to external flash.
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH 1
#define MEMFAULT_COREDUMP_STORAGE_START_ADDR 0x20000
#define MEMFAULT_COREDUMP_STORAGE_END_ADDR 0x30000

#define MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL 1

// Allows for log buffer to be captured outside of coredump. Leave disabled for now to save a
// little codespace
#define MEMFAULT_LOG_DATA_SOURCE_ENABLED 0

// For example, decide if you want to use the Gnu Build ID.
#if defined(__GNUC__)
#define MEMFAULT_USE_GNU_BUILD_ID 1
#endif

#if defined (__DA14531__)
// Tune some parameters to save additional RAM space on the DA1531

#define MEMFAULT_DATA_SOURCE_RLE_ENABLED 0
#define MEMFAULT_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED 0
#define MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED 0
#define MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED 0
#define MEMFAULT_SDK_LOG_SAVE_DISABLE 1
#endif

#ifdef __cplusplus
}
#endif
