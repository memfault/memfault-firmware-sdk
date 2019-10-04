//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handling for Cortex M based devices

#include "memfault/panics/fault_handling.h"

#include "memfault/panics/coredump.h"
#include "memfault/panics/reboot_tracking.h"
#include "memfault/core/platform/core.h"
#include "memfault_reboot_tracking_private.h"

static eMfltRebootReason s_crash_reason = kMfltRebootReason_Unknown;

// Figure out what stack was being used leading up to the exception Then call
// memfault_exception_handler with the stack used & reboot reason
#define MEMFAULT_HARDFAULT_HANDLING_ASM(_x)      \
  __asm volatile(                                \
      "tst lr, #4 \n"                            \
      "ite eq \n"                                \
      "mrseq r3, msp \n"                         \
      "mrsne r3, psp \n"                         \
      "push {r3-r11, lr} \n"                     \
      "mov r0, sp \n"                            \
      "mov r1, %0 \n"                            \
      "b memfault_exception_handler \n"          \
      :                                          \
      : "i" (_x)                                 \
                  )

// Cortex M stack on exception entry
typedef struct MEMFAULT_PACKED MfltExceptionFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t pc;
  uint32_t xpsr;
} sMfltExceptionFrame;

// The state of registers at exception entry
typedef struct MEMFAULT_PACKED MfltRegState {
  sMfltExceptionFrame *exception_frame;
  // callee saved registers
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t exc_return; // on exception entry, this value is in the LR
} sMfltRegState;

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
} sMfltCortexMRegs;

void memfault_exception_handler(sMfltRegState *regs, eMfltRebootReason reason) {
  if (s_crash_reason == kMfltRebootReason_Unknown) {
    sMfltCrashInfo info = {
      .reason = reason,
      .pc = regs->exception_frame->pc,
      .lr = regs->exception_frame->lr,
    };
    memfault_reboot_tracking_mark_crash(&info);
    s_crash_reason = info.reason;
  }

  bool fpu_stack_space_rsvd = ((regs->exc_return & (1 << 4)) == 0);
  bool stack_alignement_forced = ((regs->exception_frame->xpsr & (1 << 9)) != 0);

  uint32_t sp_prior_to_exception = (uint32_t)regs->exception_frame +
      (fpu_stack_space_rsvd ? 0x68 : 0x20);

  if (stack_alignement_forced) {
    sp_prior_to_exception += 0x4;
  }

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
  };

  memfault_coredump_save(&core_regs, sizeof(core_regs), s_crash_reason);

  memfault_platform_reboot();
  MEMFAULT_UNREACHABLE;
}

MEMFAULT_NAKED_FUNC
void HardFault_Handler(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_HardFault);
}

MEMFAULT_NAKED_FUNC
void MemoryManagement_Handler(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_MemFault);
}

MEMFAULT_NAKED_FUNC
void BusFault_Handler(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_BusFault);
}

MEMFAULT_NAKED_FUNC
void UsageFault_Handler(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_UsageFault);
}

MEMFAULT_NAKED_FUNC
void NMI_Handler(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_Assert);
}

void memfault_fault_handling_assert(void *pc, void *lr, uint32_t extra) {
  sMfltCrashInfo info = {
    .reason = kMfltRebootReason_Assert,
    .pc = (uint32_t)pc,
    .lr = (uint32_t)lr,
  };
  memfault_reboot_tracking_mark_crash(&info);
  s_crash_reason = info.reason;

  memfault_platform_halt_if_debugging();

  // NOTE: Address of NMIPENDSET is a standard (please see
  // the "Interrupt Control and State Register" section of the ARMV7-M reference manual)

  // Run coredump collection handler from NMI handler
  // Benefits:
  //   At that priority level, we can't get interrupted
  //   We can leverage the arm architecture to auto-capture register state for us
  //   If the user is using psp/msp, we start execution from a more predictable stack location
  const uint32_t nmipendset_mask = 0x1u << 31;
  volatile uint32_t *icsr = (uint32_t *)0xE000ED04;
  *icsr |= nmipendset_mask;
  __asm("isb");

  // We just pend'd a NMI interrupt which is higher priority than any other interrupt and so we
  // should not get here unless the this gets called while fault handling is _already_ in progress
  // and the NMI is running. In this situation, the best thing that can be done is rebooting the
  // system to recover it
  memfault_platform_reboot();
}
