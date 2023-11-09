//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for Mbed
#include "cmsis.h"
#include "memfault/core/compiler.h"
#include "os_tick.h"

int memfault_platform_boot(void) {
  return 0;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}

//! Example implementation. Re-define or verify this works for your platform.
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  const uint64_t tick_count = OS_Tick_GetCount();
  const uint32_t ticks_per_sec = OS_Tick_GetClock();
  return (1000 * tick_count) / ticks_per_sec;
}
