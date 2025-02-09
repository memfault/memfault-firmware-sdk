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
  volatile uint32_t *SCB_AIRCR = (uint32_t *)SCB_AIRCR_ADDR;
  *SCB_AIRCR = SCB_AIRCR_RESET_SYSTEM;

  while (1) { }  // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    { .start_addr = 0x20000000, .length = 0x800000 },
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

static uint32_t prv_read_reboot_reg(void) {
  volatile uint32_t *cmsdk_rstinfo = (uint32_t *)CMSDK_RSTINFO_ADDR;
  return (*cmsdk_rstinfo & CMSDK_RSTINFO_MASK);
}

static void prv_clear_reboot_reg(void) {
  volatile uint32_t *cmsdk_rstinfo = (uint32_t *)CMSDK_RSTINFO_ADDR;

  // Write any bit to clear
  *cmsdk_rstinfo = 1;
}

//! Decode the reset info register
//!
//! NB: QEMU does not follow the behavior specified in AN386 and always returns 0
static eMemfaultRebootReason prv_decode_reboot_reg(uint32_t reboot_reg) {
  eMemfaultRebootReason result = kMfltRebootReason_Unknown;
  if (reboot_reg & CMSDK_RSTINFO_SYSRESETREQ_MASK) {
    result = kMfltRebootReason_SoftwareReset;
  } else if (reboot_reg & CMSDK_RSTINFO_WATCHDOG_MASK) {
    result = kMfltRebootReason_HardwareWatchdog;
  } else if (reboot_reg & CMSDK_RSTINFO_LOCKUP_MASK) {
    result = kMfltRebootReason_Lockup;
  } else {
    result = kMfltRebootReason_PowerOnReset;
  }

  prv_clear_reboot_reg();
  return result;
}

void memfault_reboot_reason_get(sResetBootupInfo *reset_info) {
  uint32_t reboot_reg = prv_read_reboot_reg();
  eMemfaultRebootReason reboot_reason = prv_decode_reboot_reg(reboot_reg);

  *reset_info = (sResetBootupInfo){
    .reset_reason_reg = reboot_reg,
    .reset_reason = reboot_reason,
  };
}
