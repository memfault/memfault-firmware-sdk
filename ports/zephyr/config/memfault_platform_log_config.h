#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Zephyr port implementation of memfault_platform_log_config.h
//!
//! This file maps the Memfault SDK log macros to implementations that respect
//! the CONFIG_MEMFAULT_LOG_LEVEL Kconfig setting. Logs below the configured
//! level will be compiled out.

#include "memfault/config.h"
#include "memfault/core/log.h"
#include "memfault/core/platform/debug_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !MEMFAULT_SDK_LOG_SAVE_DISABLE
  // Note that this call will be a no-op if the system has not initialized the log module
  // by calling memfault_log_boot(). See ./log.h for more details.
  #define MEMFAULT_SDK_LOG_SAVE MEMFAULT_LOG_SAVE
#else
  #define MEMFAULT_SDK_LOG_SAVE(...)
#endif

#define _MEMFAULT_LOG_IMPL(_level, ...)         \
  do {                                          \
    MEMFAULT_SDK_LOG_SAVE(_level, __VA_ARGS__); \
    memfault_platform_log(_level, __VA_ARGS__); \
  } while (0)

// Map Memfault log levels to Zephyr log levels
// LOG_LEVEL values: 0=OFF, 1=ERR, 2=WRN, 3=INF, 4=DBG
// kMemfaultPlatformLogLevel: Debug=0, Info=1, Warning=2, Error=3

// Compile out logs below the configured level
#if CONFIG_MEMFAULT_LOG_LEVEL >= 4  // DBG
  #define MEMFAULT_LOG_DEBUG(...) _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Debug, __VA_ARGS__)
#else
  #define MEMFAULT_LOG_DEBUG(...)
#endif

#if CONFIG_MEMFAULT_LOG_LEVEL >= 3  // INF
  #define MEMFAULT_LOG_INFO(...) _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Info, __VA_ARGS__)
#else
  #define MEMFAULT_LOG_INFO(...)
#endif

#if CONFIG_MEMFAULT_LOG_LEVEL >= 2  // WRN
  #define MEMFAULT_LOG_WARN(...) _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Warning, __VA_ARGS__)
#else
  #define MEMFAULT_LOG_WARN(...)
#endif

#if CONFIG_MEMFAULT_LOG_LEVEL >= 1  // ERR
  #define MEMFAULT_LOG_ERROR(...) _MEMFAULT_LOG_IMPL(kMemfaultPlatformLogLevel_Error, __VA_ARGS__)
#else
  #define MEMFAULT_LOG_ERROR(...)
#endif

//! Only needs to be implemented when using demo component
#define MEMFAULT_LOG_RAW(...) memfault_platform_log_raw(__VA_ARGS__)

#ifdef __cplusplus
}
#endif
