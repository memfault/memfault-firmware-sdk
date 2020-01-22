//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! An example implementation of the logging memfault API for the WICED platform

#include "memfault/core/platform/debug_log.h"
#include "memfault/core/compiler.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "wwd_debug.h"

#ifndef MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

static const char *TAG MEMFAULT_UNUSED = "mflt";

static void prv_print(const char *fmt, va_list *args) {
#ifdef ENABLE_JLINK_TRACE
  if (WPRINT_PLATFORM_PERMISSION_FUNC()) {
    char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
    vsnprintf(log_buf, sizeof(log_buf), fmt, *args)
    RTT_printf("%s\n", log_buf);
  }
#else
  vprintf(fmt, *args);
  printf("\n");
#endif
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  (void)level;

  va_list args;
  va_start(args, fmt);
  prv_print(fmt, &args);
  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  prv_print(fmt, &args);
  va_end(args);
}
