//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! An example implementation of the logging memfault API for the NRF52 platform

#include "memfault/core/platform/debug_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "sdk_config.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_internal.h"

#ifndef MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

#if !defined(NRF_LOG_DEFERRED) || (NRF_LOG_DEFERRED == 1)
#  error "Example implementation assumes NRF_LOG_DEFERRED 0 (so strings print)"
#endif

#if !defined(NRF_LOG_BACKEND_RTT_ENABLED) || (NRF_LOG_BACKEND_RTT_ENABLED == 0)
#  error "Example implementation assumes NRF_LOG_BACKEND_RTT_ENABLED 1. Other backends may not work as expected"
#endif

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  strcpy(log_buf, "MFLT: ");
  char *write_ptr = &log_buf[0] + strlen(log_buf);

  vsnprintf(write_ptr, sizeof(log_buf) - strlen(log_buf), fmt, args);

  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      NRF_LOG_DEBUG("%s", log_buf);
      break;

    case kMemfaultPlatformLogLevel_Info:
      NRF_LOG_INFO("%s", log_buf);
      break;

    case kMemfaultPlatformLogLevel_Warning:
      NRF_LOG_WARNING("%s", log_buf);
      break;

    case kMemfaultPlatformLogLevel_Error:
      NRF_LOG_ERROR("%s", log_buf);
      break;
    default:
      break;
  }

  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  NRF_LOG_INTERNAL_RAW_INFO("%s\n", log_buf);

  // Delay a little bit to give the host a chance to drain the buffers and
  // avoid Segger RTT overruns (data will be dropped on the floor otherwise).
  // NRF_LOG_FLUSH() is a no-op for the RTT logging backend unfortunately.
  nrf_delay_ms(1);

  va_end(args);
}

void memfault_platform_hexdump(eMemfaultPlatformLogLevel level, const void *data, size_t data_len) {
  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      NRF_LOG_HEXDUMP_DEBUG(data, data_len);
      break;

    case kMemfaultPlatformLogLevel_Info:
      NRF_LOG_HEXDUMP_INFO(data, data_len);
      break;

      // v15 SDK is broken for warning level hex log
    case kMemfaultPlatformLogLevel_Warning:
    case kMemfaultPlatformLogLevel_Error:
      NRF_LOG_HEXDUMP_ERROR(data, data_len);
      break;
    default:
      break;
  }
}
