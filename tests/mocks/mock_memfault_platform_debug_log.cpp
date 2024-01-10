//! @file
//!
//! Mock implementation of memfault logging subsystem which can be used
//! when asserting that logs of certain formats are generated from unit tests

#include "mock_memfault_platform_debug_log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CppUTestExt/MockSupport.h"
#include "memfault/core/compiler.h"
#include "memfault/core/log.h"
#include "memfault/core/platform/debug_log.h"

#define LOG_BUFFER_SIZE (512)

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  char log_buf[LOG_BUFFER_SIZE];

  va_list args;
  va_start(args, fmt);
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  va_end(args);

  // Let's avoid dealing with the va_list, we usually really care only about the resulting string anyway:
  mock().actualCall(__func__)
    .withLongIntParameter("level", level)
    .withStringParameter("output", log_buf);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  char log_buf[LOG_BUFFER_SIZE];

  va_list args;
  va_start(args, fmt);
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  va_end(args);

  // Let's avoid dealing with the va_list, we usually really care only about the resulting string anyway:
  mock().actualCall(__func__)
    .withStringParameter("output", log_buf);
}

void memfault_platform_log_set_mock(eMemfaultPlatformLogLevel level, const char *const lines[],
                                    size_t num_lines) {
  for (unsigned int i = 0; i < num_lines; ++i) {
    mock()
      .expectOneCall("memfault_platform_log")
      .withIntParameter("level", level)
      .withStringParameter("output", lines[i]);
  }
}
