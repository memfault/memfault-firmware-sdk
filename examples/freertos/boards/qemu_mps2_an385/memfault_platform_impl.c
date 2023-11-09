//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/components.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/freertos_coredump.h"

#define SCB_AIRCR_ADDR (0xE000ED0C)
#define SCB_AIRCR_VECTKEY_WRITE (0x5FAUL << 16U)
#define SCB_AIRCR_SYSRESETREQ (1U << 2)
#define SCB_AIRCR_RESET_SYSTEM (SCB_AIRCR_VECTKEY_WRITE | SCB_AIRCR_SYSRESETREQ)

#define CMSDK_RSTINFO_ADDR (0x4001F010)
#define CMSDK_RSTINFO_MASK (0x07)
#define CMSDK_RSTINFO_SYSRESETREQ_MASK (1 << 0)
#define CMSDK_RSTINFO_WATCHDOG_MASK (1 << 1)
#define CMSDK_RSTINFO_LOCKUP_MASK (1 << 2)

#define MEMFAULT_COREDUMP_MAX_TASK_REGIONS ((MEMFAULT_PLATFORM_MAX_TRACKED_TASKS)*2)

void memfault_platform_reboot(void) {
  volatile uint32_t *SCB_AIRCR = (uint32_t *)SCB_AIRCR_ADDR;
  *SCB_AIRCR = SCB_AIRCR_RESET_SYSTEM;

  while (1) {
  }  // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    {.start_addr = 0x20000000, .length = 0x800000},
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

static uint32_t prv_read_psp_reg(void) {
  uint32_t reg_val;
  __asm volatile("mrs %0, psp" : "=r"(reg_val));
  return reg_val;
}

static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_COREDUMP_MAX_TASK_REGIONS +
                                              2   /* active stack(s) */
                                              + 1 /* _kernel variable */
                                              + 1 /* __memfault_capture_start */
                                              + 2 /* s_task_tcbs + s_task_watermarks */
];

extern uint32_t __memfault_capture_bss_end;
extern uint32_t __memfault_capture_bss_start;

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  int region_idx = 0;
  const size_t active_stack_size_to_collect = 512;

  // first, capture the active stack (and ISR if applicable)
  const bool msp_was_active = (crash_info->exception_reg_state->exc_return & (1 << 2)) == 0;

  size_t stack_size_to_collect = memfault_platform_sanitize_address_range(
    crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

  s_coredump_regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(crash_info->stack_address, stack_size_to_collect);
  region_idx++;

  if (msp_was_active) {
    // System crashed in an ISR but the running task state is on PSP so grab that too
    void *psp = (void *)prv_read_psp_reg();

    // Collect a little bit more stack for the PSP since there is an
    // exception frame that will have been stacked on it as well
    const uint32_t extra_stack_bytes = 128;
    stack_size_to_collect = memfault_platform_sanitize_address_range(
      psp, active_stack_size_to_collect + extra_stack_bytes);
    s_coredump_regions[region_idx] =
      MEMFAULT_COREDUMP_MEMORY_REGION_INIT(psp, stack_size_to_collect);
    region_idx++;
  }

  // Scoop up memory regions necessary to perform unwinds of the FreeRTOS tasks
  const size_t memfault_region_size =
    (uint32_t)&__memfault_capture_bss_end - (uint32_t)&__memfault_capture_bss_start;

  s_coredump_regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&__memfault_capture_bss_start, memfault_region_size);
  region_idx++;

  region_idx += memfault_freertos_get_task_regions(
    &s_coredump_regions[region_idx], MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

  *num_regions = region_idx;
  return &s_coredump_regions[0];
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
//! NB: QEMU does not follow the behavior specified in AN385 and always returns 0
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
