//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A fake implementation simulating platform logs which can be used for unit tests

#include "memfault/core/platform/debug_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "memfault/core/log.h"
#include "memfault/core/compiler.h"

static const char *prv_severity_level_to_str(eMemfaultPlatformLogLevel level) {
  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      return "D";
    case kMemfaultPlatformLogLevel_Info:
      return "I";
    case kMemfaultPlatformLogLevel_Warning:
      return "W";
    case kMemfaultPlatformLogLevel_Error:
      return "E";
    case kMemfaultPlatformLogLevel_NumLevels: // silence error with -Wswitch-enum
    default:
      return "U";
  }
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  strcpy(log_buf, "MFLT: ");
  char *write_ptr = &log_buf[0] + strlen(log_buf);
  vsnprintf(write_ptr, sizeof(log_buf) - strlen(log_buf), fmt, args);

  printf("[%s] %s\n", prv_severity_level_to_str(level), log_buf);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("%s", log_buf);
}

void memfault_platform_hexdump(MEMFAULT_UNUSED eMemfaultPlatformLogLevel level,
                               MEMFAULT_UNUSED const void *data, MEMFAULT_UNUSED size_t data_len) {
  // No fake impl yet!
}
