//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handling for Cortex M based devices

#include "memfault/panics/fault_handling.h"

#include "memfault/core/platform/core.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"
#include "memfault_reboot_tracking_private.h"

static eMfltResetReason s_crash_reason = kMfltRebootReason_Unknown;

typedef struct MEMFAULT_PACKED MfltCortexMRegs {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t sp;
  uint32_t lr;
  uint32_t pc;
  uint32_t psr;
  uint32_t msp;
  uint32_t psp;
} sMfltCortexMRegs;

size_t memfault_coredump_storage_compute_size_required(void) {
  // actual values don't matter since we are just computing the size
  sMfltCortexMRegs core_regs = { 0 };
  sMemfaultCoredumpSaveInfo save_info = {
    .regs = &core_regs,
    .regs_size = sizeof(core_regs),
    .trace_reason = kMfltRebootReason_UnknownError,
  };

  sCoredumpCrashInfo info = {
    // we'll just pass the current stack pointer, value shouldn't matter
    .stack_address = (void *)&core_regs,
    .trace_reason = save_info.trace_reason,
  };
  save_info.regions = memfault_platform_coredump_get_regions(&info, &save_info.num_regions);

  return memfault_coredump_get_save_size(&save_info);
}

static uint32_t prv_read_psp_reg(void) {
  uint32_t reg_val;
  __asm volatile ("mrs %0, psp"  : "=r" (reg_val));
  return reg_val;
}

static uint32_t prv_read_msp_reg(void) {
  uint32_t reg_val;
  __asm volatile ("mrs %0, msp"  : "=r" (reg_val));
  return reg_val;
}

void memfault_fault_handler(const sMfltRegState *regs, eMfltResetReason reason) {
  if (s_crash_reason == kMfltRebootReason_Unknown) {
    sMfltRebootTrackingRegInfo info = {
      .pc = regs->exception_frame->pc,
      .lr = regs->exception_frame->lr,
    };
    memfault_reboot_tracking_mark_reset_imminent(reason, &info);
    s_crash_reason = reason;
  }

  const bool fpu_stack_space_rsvd = ((regs->exc_return & (1 << 4)) == 0);
  const bool stack_alignment_forced = ((regs->exception_frame->xpsr & (1 << 9)) != 0);

  uint32_t sp_prior_to_exception =
      (uint32_t)regs->exception_frame + (fpu_stack_space_rsvd ? 0x68 : 0x20);

  if (stack_alignment_forced) {
    sp_prior_to_exception += 0x4;
  }

  const bool msp_was_active = (regs->exc_return & (1 << 3)) == 0;

  sMfltCortexMRegs core_regs = {
    .r0 = regs->exception_frame->r0,
    .r1 = regs->exception_frame->r1,
    .r2 = regs->exception_frame->r2,
    .r3 = regs->exception_frame->r3,
    .r4 = regs->r4,
    .r5 = regs->r5,
    .r6 = regs->r6,
    .r7 = regs->r7,
    .r8 = regs->r8,
    .r9 = regs->r9,
    .r10 = regs->r10,
    .r11 = regs->r11,
    .r12 = regs->exception_frame->r12,
    .sp = sp_prior_to_exception,
    .lr = regs->exception_frame->lr,
    .pc = regs->exception_frame->pc,
    .psr = regs->exception_frame->xpsr,
    .msp = msp_was_active ? sp_prior_to_exception : prv_read_msp_reg(),
    .psp = !msp_was_active ? sp_prior_to_exception : prv_read_psp_reg(),
  };

  sMemfaultCoredumpSaveInfo save_info = {
    .regs = &core_regs,
    .regs_size = sizeof(core_regs),
    .trace_reason = s_crash_reason,
  };

  sCoredumpCrashInfo info = {
    .stack_address = (void *)sp_prior_to_exception,
    .trace_reason = save_info.trace_reason,
  };
  save_info.regions = memfault_platform_coredump_get_regions(&info, &save_info.num_regions);

  const bool coredump_saved = memfault_coredump_save(&save_info);
  if (coredump_saved) {
    memfault_reboot_tracking_mark_coredump_saved();
  }

  memfault_platform_reboot();
  MEMFAULT_UNREACHABLE;
}

// Figure out what stack was being used leading up to the exception Then call
// memfault_fault_handler with the stack used & reboot reason
#define MEMFAULT_HARDFAULT_HANDLING_ASM(_x)      \
  __asm volatile(                                \
      "tst lr, #4 \n"                            \
      "ite eq \n"                                \
      "mrseq r3, msp \n"                         \
      "mrsne r3, psp \n"                         \
      "push {r3-r11, lr} \n"                     \
      "mov r0, sp \n"                            \
      "ldr r1, =%0 \n"                           \
      "b memfault_fault_handler \n"              \
      :                                          \
      : "i" (_x)                                 \
   )

// By default, exception handlers use CMSIS naming conventions.
// However, if needed, each handler can be renamed using the following
// preprocessor defines:

#ifndef MEMFAULT_EXC_HANDLER_HARD_FAULT
#  define MEMFAULT_EXC_HANDLER_HARD_FAULT HardFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT
#  define MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT MemoryManagement_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_BUS_FAULT
#  define MEMFAULT_EXC_HANDLER_BUS_FAULT BusFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_USAGE_FAULT
#  define MEMFAULT_EXC_HANDLER_USAGE_FAULT UsageFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_NMI
#  define MEMFAULT_EXC_HANDLER_NMI NMI_Handler
#endif

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_HARD_FAULT(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_HardFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_MemFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_BUS_FAULT(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_BusFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_USAGE_FAULT(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_UsageFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_NMI(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_Assert);
}

void memfault_fault_handling_assert(void *pc, void *lr, uint32_t extra) {
  sMfltRebootTrackingRegInfo info = {
    .pc = (uint32_t)pc,
    .lr = (uint32_t)lr,
  };
  s_crash_reason = kMfltRebootReason_Assert;
  memfault_reboot_tracking_mark_reset_imminent(s_crash_reason, &info);


  memfault_platform_halt_if_debugging();

  // NOTE: Address of NMIPENDSET is a standard (please see
  // the "Interrupt Control and State Register" section of the ARMV7-M reference manual)

  // Run coredump collection handler from NMI handler
  // Benefits:
  //   At that priority level, we can't get interrupted
  //   We can leverage the arm architecture to auto-capture register state for us
  //   If the user is using psp/msp, we start execution from a more predictable stack location
  const uint32_t nmipendset_mask = 0x1 << 31;
  volatile uint32_t *icsr = (uint32_t *)0xE000ED04;
  *icsr |= nmipendset_mask;
  __asm("isb");

  // We just pend'd a NMI interrupt which is higher priority than any other interrupt and so we
  // should not get here unless the this gets called while fault handling is _already_ in progress
  // and the NMI is running. In this situation, the best thing that can be done is rebooting the
  // system to recover it
  memfault_platform_reboot();
}
