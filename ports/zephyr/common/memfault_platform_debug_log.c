//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Maps memfault platform logging API to zephyr kernel logs

#include "memfault/core/platform/debug_log.h"

//

#include <logging/log.h>
#include <stdio.h>

#include "memfault/config.h"
#include "memfault/ports/zephyr/version.h"
#include "zephyr_release_specific_headers.h"

LOG_MODULE_REGISTER(mflt, CONFIG_MEMFAULT_LOG_LEVEL);

#ifndef MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES
  #define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

//! Translate Memfault logs to Zephyr logs.
void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  char *log_str = log_buf;

#if !MEMFAULT_ZEPHYR_VERSION_GT(3, 1)
  #if !defined(CONFIG_LOG2)
  // Before zephyr 3.1, LOG was a different option from LOG2 and required
  // manually duplicating string argument values.
  log_str = log_strdup(log_buf);
  #endif
#endif

  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      LOG_DBG("%s", log_str);
      break;

    case kMemfaultPlatformLogLevel_Info:
      LOG_INF("%s", log_str);
      break;

    case kMemfaultPlatformLogLevel_Warning:
      LOG_WRN("%s", log_str);
      break;

    case kMemfaultPlatformLogLevel_Error:
      LOG_ERR("%s", log_str);
      break;

    default:
      LOG_ERR("??? %s", log_str);
      break;
  }

  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

#define ZEPHYR_VERSION_GTE(major, minor) \
  ((KERNEL_VERSION_MAJOR > (major)) ||   \
   ((KERNEL_VERSION_MAJOR == (major)) && (KERNEL_VERSION_MINOR >= (minor))))

#if ZEPHYR_VERSION_GTE(3, 0)
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  LOG_PRINTK("%s", log_buf);
#else
  log_printk("%s\n", args);
#endif

  va_end(args);
}
