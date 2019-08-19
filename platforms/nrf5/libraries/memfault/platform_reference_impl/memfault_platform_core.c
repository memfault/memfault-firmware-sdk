//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for the NRF52

#include "app_util_platform.h"
#include "memfault/core/compiler.h"

int memfault_platform_boot(void) {
  return 0;
}

void memfault_platform_halt_if_debugging(void) {
  NRF_BREAKPOINT_COND;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
