//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for Mbed
#include "cmsis.h"

#include "memfault/core/compiler.h"

int memfault_platform_boot(void) {
  return 0;
}

void memfault_platform_halt_if_debugging(void) {
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
    __BKPT(0);
  }
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
