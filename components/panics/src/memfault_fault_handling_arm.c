//! @file
//!
//! Copyright (c) Memfault, Inc.
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

typedef MEMFAULT_PACKED_STRUCT MfltCortexMRegs {
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

#if defined(__CC_ARM)

static uint32_t prv_read_psp_reg(void) {
  register uint32_t reg_val  __asm("psp");
  return reg_val;
}

static uint32_t prv_read_msp_reg(void) {
  register uint32_t reg_val __asm("msp");
  return reg_val;
}

#elif defined(__GNUC__) || defined(__clang__) || defined(__ICCARM__)

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

#else
#  error "New compiler to add support for!"
#endif

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



// The fault handling shims below figure out what stack was being used leading up to the exception,
// build the sMfltRegState argument and pass that as well as the reboot reason to memfault_fault_handler


#if defined(__CC_ARM)

// armcc emits a define for the CPU target. Use that information to decide whether or not to pick
// up the ARMV6M port by default
#if defined(__TARGET_CPU_CORTEX_M0)
#define MEMFAULT_USE_ARMV6M_FAULT_HANDLER 1
#endif

#if !defined(MEMFAULT_USE_ARMV6M_FAULT_HANDLER)
__asm __forceinline void memfault_fault_handling_shim(int reason) {
  extern memfault_fault_handler;
  tst lr, #4
  ite eq
  mrseq r3, msp
  mrsne r3, psp
  push {r3-r11, lr}
  mov r1, r0
  mov r0, sp
  b memfault_fault_handler
  ALIGN
}
#else

__asm __forceinline void memfault_fault_handling_shim(int reason) {
  extern memfault_fault_handler;
  PRESERVE8
  mov r1, lr
  movs r2, #4
  tst  r1,r2
  mrs r12, msp
  beq msp_active_at_crash
  mrs r12, psp
msp_active_at_crash mov r3, r11
  mov r2, r10
  mov r1, r9
  mov r9, r0
  mov r0, r8
  push {r0-r3, lr}
  mov r3, r12
  push {r3-r7}
  mov r0, sp
  mov r1, r9
  ldr r2, =memfault_fault_handler
  bx r2
  ALIGN
}
#endif

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_HARD_FAULT(void) {
  ldr r0, =0x9400 // kMfltRebootReason_HardFault
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT(void) {
  ldr r0, =0x9200 // kMfltRebootReason_MemFault
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_BUS_FAULT(void) {
  ldr r0, =0x9100 // kMfltRebootReason_BusFault
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_USAGE_FAULT(void) {
  ldr r0, =0x9300 // kMfltRebootReason_UsageFault
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_NMI(void) {
  ldr r0, =0x8001 // kMfltRebootReason_Assert
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_WATCHDOG(void) {
  ldr r0, =0x8002 // kMfltRebootReason_Watchdog
  ldr r1, =memfault_fault_handling_shim
  bx r1
  ALIGN
}

#elif defined(__GNUC__) || defined(__clang__)

#if defined(__ARM_ARCH) && (__ARM_ARCH == 6)
#define MEMFAULT_USE_ARMV6M_FAULT_HANDLER 1
#endif

#if !defined(MEMFAULT_USE_ARMV6M_FAULT_HANDLER)
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
#else
#define MEMFAULT_HARDFAULT_HANDLING_ASM(_x)      \
  __asm volatile(                                \
      "mov r0, lr \n"                            \
      "movs r1, #4 \n"                           \
      "tst  r0,r1 \n"                            \
      "mrs r12, msp \n"                          \
      "beq msp_active_at_crash_%= \n"            \
      "mrs r12, psp \n"                          \
      "msp_active_at_crash_%=: \n"               \
      "mov r0, r8 \n"                            \
      "mov r1, r9 \n"                            \
      "mov r2, r10 \n"                           \
      "mov r3, r11 \n"                           \
      "push {r0-r3, lr} \n"                      \
      "mov r3, r12 \n"                           \
      "push {r3-r7} \n"                          \
      "mov r0, sp \n"                            \
      "ldr r1, =%0 \n"                           \
      "b memfault_fault_handler \n"              \
      :                                          \
      : "i" (_x)                                 \
   )
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

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_WATCHDOG(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_Watchdog);
}

#elif defined(__ICCARM__)

#if __ARM_ARCH == 6
#define MEMFAULT_USE_ARMV6M_FAULT_HANDLER 1
#endif

#if !defined(MEMFAULT_USE_ARMV6M_FAULT_HANDLER)
#define MEMFAULT_HARDFAULT_HANDLING_ASM(_x)      \
  __asm volatile(                                \
      "tst lr, #4 \n"                            \
      "ite eq \n"                                \
      "mrseq r3, msp \n"                         \
      "mrsne r3, psp \n"                         \
      "push {r3-r11, lr} \n"                     \
      "mov r0, sp \n"                            \
      "mov r1, %0 \n"                            \
      "b memfault_fault_handler \n"              \
      :                                          \
      : "i" (_x)                                 \
   )

#else

// Note: Below IAR will build the enum value
// as part of the prologue to the asm statement and
// place the value in r0
#define MEMFAULT_HARDFAULT_HANDLING_ASM(_x)      \
  __asm volatile(                                \
      "mov r1, lr \n"                            \
      "movs r2, #4 \n"                           \
      "tst  r1,r2 \n"                            \
      "mrs r12, msp \n"                          \
      "beq msp_active_at_crash \n"               \
      "mrs r12, psp \n"                          \
      "msp_active_at_crash: \n"                  \
      "mov r3, r11 \n"                           \
      "mov r2, r10 \n"                           \
      "mov r1, r9 \n"                            \
      "mov r9, r0 \n"                            \
      "mov r0, r8 \n"                            \
      "push {r0-r3, lr} \n"                      \
      "mov r3, r12 \n"                           \
      "push {r3-r7} \n"                          \
      "mov r0, sp \n"                            \
      "mov r1, r9 \n"                            \
      "ldr r2, =memfault_fault_handler \n"       \
      "bx r2 \n"                                 \
      :                                          \
      : "r" (_x)                                 \
   )

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

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_WATCHDOG(void) {
  MEMFAULT_HARDFAULT_HANDLING_ASM(kMfltRebootReason_Watchdog);
}

#else
#  error "New compiler to add support for!"
#endif


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
