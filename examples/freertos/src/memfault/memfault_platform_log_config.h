//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Shim Memfault logs over to platform logging handler, inserting any
//! additional metadata.

#pragma once

#include <string.h>
#include <time.h>

#include "memfault/config.h"
#include "memfault/core/log.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/debug_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _MEMFAULT_LOG_IMPL(_level, str_level, fmt, ...)                                           \
  do {                                                                                            \
    MEMFAULT_SDK_LOG_SAVE(_level, fmt, ##__VA_ARGS__);                                            \
    /* Insert a timestamp and system uptime */                                                    \
    time_t time_now = time(NULL);                                                                 \
    struct tm *tm_time = gmtime(&time_now);                                                       \
    char time_str[32];                                                                            \
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm_time);                          \
    uint32_t uptime = memfault_platform_get_time_since_boot_ms();                                 \
    memfault_platform_log(_level, "%s|%lu " #str_level " " fmt, time_str, uptime, ##__VA_ARGS__); \
  } while (0)

#define MEMFAULT_LOG_DEBUG(fmt, ...) \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Debug, D, fmt, ##__VA_ARGS__)
#define MEMFAULT_LOG_INFO(fmt, ...) \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Info, I, fmt, ##__VA_ARGS__)
#define MEMFAULT_LOG_WARN(fmt, ...) \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Warning, W, fmt, ##__VA_ARGS__)
#define MEMFAULT_LOG_ERROR(fmt, ...) \
  _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Error, E, fmt, ##__VA_ARGS__)

//! Only needs to be implemented when using demo component
#define MEMFAULT_LOG_RAW(...) memfault_platform_log_raw(__VA_ARGS__)

#ifdef __cplusplus
}
#endif
