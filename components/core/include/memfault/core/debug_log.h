#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Log utility used within the memfault SDK. When enabled, logs will be emitted to help a user
//! understand what is happening in the library. The Memfault SDK uses logs sparingly
//! to better call out glaring configuration issues and errors.
//!
//! If your system does not have logging infrastructure in place, the subsystem can also be
//! leveraged for logging within your platform. In that situation we suggest lightly wrapping
//! the macros in your own file for future flexibility:
//!
//! #include "memfault/core/debug_log.h"
//! #define YOUR_PLATFORM_LOG_DEBUG(...) MEMAULT_LOG_DEBUG(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_INFO(...) MEMAULT_LOG_INFO(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_WARN(...) MEMAULT_LOG_WARN(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_ERROR(...) MEMAULT_LOG_ERROR(__VA_ARGS__)

#include "memfault/core/log.h"
#include "memfault/core/platform/debug_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shouldn't typically be needed but allows for persisting of MEMFAULT_LOG_*'s
// to be disabled via a CFLAG: CFLAGS += -DMEMFAULT_SDK_LOG_SAVE_DISABLE=1
#if !defined(MEMFAULT_SDK_LOG_SAVE_DISABLE)
  #define MEMFAULT_SDK_LOG_SAVE_DISABLE 0
#endif

#if !MEMFAULT_SDK_LOG_SAVE_DISABLE
// Note that this call will be a no-op if the system has not initialized the log module
// by calling memfault_log_boot(). See ./log.h for more details.
#define MEMFAULT_SDK_LOG_SAVE MEMFAULT_LOG_SAVE
#else
#define MEMFAULT_SDK_LOG_SAVE(...)
#endif

#define _MEMFAULT_LOG_IMPL(_level, ...)             \
  do {                                              \
    MEMFAULT_SDK_LOG_SAVE(_level, __VA_ARGS__);     \
    memfault_platform_log(_level, __VA_ARGS__);     \
  } while (0)

#define MEMFAULT_LOG_DEBUG(...)                                             \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Debug, __VA_ARGS__)

#define MEMFAULT_LOG_INFO(...)                                              \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Info, __VA_ARGS__)

#define MEMFAULT_LOG_WARN(...)                                              \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Warning, __VA_ARGS__)

#define MEMFAULT_LOG_ERROR(...)                                             \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Error, __VA_ARGS__)

#define MEMFAULT_LOG_RAW(...)                                               \
  memfault_platform_log_raw(__VA_ARGS__)

#ifdef __cplusplus
}
#endif
