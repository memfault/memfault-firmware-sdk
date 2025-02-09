//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/components.h"

#define SCB_AIRCR_ADDR (0xE000ED0C)
#define SCB_AIRCR_VECTKEY_WRITE (0x5FAUL << 16U)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)
#define SCB_AIRCR_RESET_SYSTEM (SCB_AIRCR_VECTKEY_WRITE | SCB_AIRCR_SYSRESETREQ)

#define CMSDK_RSTINFO_ADDR (0x4001F010)
#define CMSDK_RSTINFO_MASK (0x07)
#define CMSDK_RSTINFO_SYSRESETREQ_MASK (1 << 0)
#define CMSDK_RSTINFO_WATCHDOG_MASK (1 << 1)
#define CMSDK_RSTINFO_LOCKUP_MASK (1 << 2)

void memfault_platform_reboot(void) {
  // // just insert memfault breakpoint
  // __asm volatile("bkpt #0");

  volatile uint32_t *SCB_AIRCR = (uint32_t *)SCB_AIRCR_ADDR;
  *SCB_AIRCR = SCB_AIRCR_RESET_SYSTEM;

  while (1) { }  // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    { .start_addr = 0x38000000, .length = 64 * 1024 },
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

void memfault_reboot_reason_get(sResetBootupInfo *reset_info) {
  // an505 doesn't seem to provide a reset reason register (an385 does, RSTINFO
  // register in the CMSDK system control registers)
  uint32_t reboot_reg = 0x1234;
  eMemfaultRebootReason reboot_reason = kMfltRebootReason_Unknown;

  *reset_info = (sResetBootupInfo){
    .reset_reason_reg = reboot_reg,
    .reset_reason = reboot_reason,
  };
}
