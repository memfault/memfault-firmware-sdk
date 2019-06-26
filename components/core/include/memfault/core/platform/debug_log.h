#pragma once

//! @file
//!
//! @brief
//! APIs that need to be implemented in order to enable logging within the memfault SDK
//!
//! The memfault SDK uses logs sparingly to communicate useful diagnostic information to the
//! integrator of the library

#include <stddef.h>

#include "memfault/core/compiler.h"

typedef enum {
  kMemfaultPlatformLogLevel_Debug,
  kMemfaultPlatformLogLevel_Info,
  kMemfaultPlatformLogLevel_Warning,
  kMemfaultPlatformLogLevel_Error,
} eMemfaultPlatformLogLevel;

//! Routine for displaying (or capturing) a log.
//!
//! @note it's expected that the implementation will terminate the log with a newline
//! @note Even if there is no UART or RTT Console, it's worth considering adding an logging
//! implementation that writes to RAM or flash which allows for post-mortem analysis
MEMFAULT_PRINTF_LIKE_FUNC(2, 3)
void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...);

//! Routine for displaying (or capturing) hexdumps
void memfault_platform_hexdump(eMemfaultPlatformLogLevel level, const void *data, size_t data_len);
