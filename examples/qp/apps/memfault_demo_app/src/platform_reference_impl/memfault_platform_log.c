//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief
//! An example implementation of the logging Memfault API
#include "memfault/core/platform/debug_log.h"

#include "bsp.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

static void prv_send_log_to_uart(const char *str, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    bsp_send_char_over_uart(str[i]);
  }
  bsp_send_char_over_uart('\n');
}

static void prv_log(const char *fmt, va_list *args) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  const size_t size = vsnprintf(log_buf, sizeof(log_buf), fmt, *args);
  prv_send_log_to_uart(log_buf, size);
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  prv_log(fmt, &args);
  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  prv_log(fmt, &args);
  va_end(args);
}
