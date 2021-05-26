//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! An example implementation of overriding the Memfault logging macros by
//! placing definitions in memfault_platform_log_config.h and adding
//! -DMEMFAULT_PLATFORM_HAS_LOG_CONFIG=1 to the compiler flags

#pragma once

#include "memfault/core/compiler.h"

#include "sdk_config.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"

//! Note: NRF_LOG_FLUSH() needs to be called if NRF_LOG_DEFERRED=1 in order
//! for string formatters to print
#define _MEMFAULT_PORT_LOG_IMPL(_level, mflt_level, fmt, ...)   \
  do {                                              \
    MEMFAULT_SDK_LOG_SAVE(mflt_level, fmt, ## __VA_ARGS__);     \
    NRF_LOG_##_level("MFLT: " fmt, ## __VA_ARGS__); \
    NRF_LOG_FLUSH();                                \
  } while (0)

#define MEMFAULT_LOG_DEBUG(fmt, ...) \
  _MEMFAULT_PORT_LOG_IMPL(DEBUG, kMemfaultPlatformLogLevel_Debug, fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_INFO(fmt, ...) \
  _MEMFAULT_PORT_LOG_IMPL(INFO, kMemfaultPlatformLogLevel_Info, fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_WARN(fmt, ...) \
  _MEMFAULT_PORT_LOG_IMPL(WARNING, kMemfaultPlatformLogLevel_Warning, fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_ERROR(fmt, ...) \
  _MEMFAULT_PORT_LOG_IMPL(ERROR, kMemfaultPlatformLogLevel_Error, fmt, ## __VA_ARGS__)


//! Note: nrf_delay_ms() is called to give the host a chance to drain the buffers and avoid Segger
//! RTT overruns (data will be dropped on the floor otherwise).  NRF_LOG_FLUSH() is a no-op for the
//! RTT logging backend unfortunately.
#define MEMFAULT_LOG_RAW(fmt, ...)                                      \
  do {                                                                  \
    NRF_LOG_INTERNAL_RAW_INFO(fmt "\n", ## __VA_ARGS__);                \
    NRF_LOG_FLUSH();                                                    \
    nrf_delay_ms(1);                                                    \
  } while (0)
