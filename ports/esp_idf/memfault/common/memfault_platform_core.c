//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdlib.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/sdk_assert.h"

#ifndef ESP_PLATFORM
#  error "The port assumes the esp-idf is in use!"
#endif

#include "soc/cpu.h"

MEMFAULT_NORETURN void memfault_sdk_assert_func_noreturn(void) {
  // Note: The esp-idf implements abort which will invoke the esp-idf coredump handler as well as a
  // chip reboot so we just piggback off of that
  abort();
}

void memfault_platform_halt_if_debugging(void) {
  if (esp_cpu_in_ocd_debug_mode()) {
    MEMFAULT_BREAKPOINT();
  }
}
