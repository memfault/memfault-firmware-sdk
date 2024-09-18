//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/core/compiler.h"
#include "memfault/core/platform/core.h"
#include "stm32h7xx_hal.h"

int memfault_platform_boot(void) {
  return 0;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
