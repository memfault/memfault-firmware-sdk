//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handling for ARMv7-A/R based devices

#include "memfault/core/compiler.h"

#if MEMFAULT_COMPILER_ARM_V7_A_R

//! Only ARMv7-R is officially supported, notify the user if compiling for
//! ARMv7-A
#if defined(__ARM_ARCH_7A__)
  #pragma message \
    "ARMv7-A is not officially supported yet by Memfault. Please contact support@memfault.com with questions!"
#endif

#include "memfault/components.h"
#include "memfault/panics/arch/arm/v7_a_r.h"
#include "memfault/panics/coredump_impl.h"

//! These are the default exception handler function names, which need to be
//! registered in the interrupt vector table for Memfault fault handling to
//! work.

#if !defined(MEMFAULT_EXC_HANDLER_UNDEFINED_INSTRUCTION)
  #define MEMFAULT_EXC_HANDLER_UNDEFINED_INSTRUCTION UndefinedInstruction_Handler
#endif

#if !defined(MEMFAULT_EXC_HANDLER_DATA_ABORT)
  #define MEMFAULT_EXC_HANDLER_DATA_ABORT DataAbort_Handler
#endif

#if !defined(MEMFAULT_EXC_HANDLER_PREFETCH_ABORT)
  #define MEMFAULT_EXC_HANDLER_PREFETCH_ABORT PrefetchAbort_Handler
#endif

extern void MEMFAULT_EXC_HANDLER_UNDEFINED_INSTRUCTION(void);
extern void MEMFAULT_EXC_HANDLER_DATA_ABORT(void);
extern void MEMFAULT_EXC_HANDLER_PREFETCH_ABORT(void);

static eMemfaultRebootReason s_crash_reason = kMfltRebootReason_Unknown;

//! These are the regs pushed by the exception wrapper, which are transcribed
//! into 'struct MfltRegState' for the coredump.
MEMFAULT_PACKED_STRUCT ExceptionPushedRegs {
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
  uint32_t sp;  // R13
  uint32_t lr;  // R14
  uint32_t cpsr;
};

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  *num_regions = 0;
  return NULL;
}

bool memfault_arch_is_inside_isr(void) {
  // read the CPSR register and check if the CPU is in User or System mode. All
  // other modes are exception/interrupt modes. See the ARMv7-A/R Architecture
  // Reference Manual section "B1.3.1 ARM processor modes" for these values.
  #define CPSR_MODE_msk 0x1f
  #define CPSR_USER_msk 0x10
  #define CPSR_SUPERVISOR_msk 0x13
  #define CPSR_SYSTEM_msk 0x1f

  uint32_t cpsr;
  __asm volatile("mrs %[cpsr], cpsr" : [cpsr] "=r" (cpsr));

  const uint32_t mode = cpsr & CPSR_MODE_msk;

  const bool in_user_mode = (mode == CPSR_USER_msk);
  const bool in_system_mode = (mode == CPSR_SYSTEM_msk);
  const bool in_supervisor_mode = (mode == CPSR_SUPERVISOR_msk);

  return !(in_user_mode || in_system_mode || in_supervisor_mode);
}

#if defined(__GNUC__)
// Coprocessor access macro. This mnemonic is based on:
// https://github.com/ARM-software/CMSIS_5/blob/5.9.0/CMSIS/Core_A/Include/cmsis_gcc.h#L838
#define __get_CP(cp, op1, Rt, CRn, CRm, op2)                               \
  __asm volatile("mrc p" #cp ", " #op1 ", %0, c" #CRn ", c" #CRm ", " #op2 \
                  : "=r"(Rt)                                               \
                  :                                                        \
                  : "memory")
static uint32_t __get_DBGDSCR(void) {
  uint32_t result;
  __get_CP(14, 0, result, 0, 1, 0);
  return result;
}
#else
  #error "Unsupported compiler"
#endif

void memfault_platform_halt_if_debugging(void) {
  // Check for the "Halting Debug Enable" bit in the DBGDSCR register. If set,
  // we are in a debugger and should break.
  uint32_t dbgdscr = __get_DBGDSCR();
  #define DBGDSCR_HDBGen_msk (1 << 14)

  if (dbgdscr & DBGDSCR_HDBGen_msk) {
    // NB: A breakpoint with value 'M' (77) for easy disambiguation from other breakpoints that may
    // be used by the system.

    __asm("BKPT 77");
  }
}

size_t memfault_coredump_storage_compute_size_required(void) {
  // actual values don't matter since we are just computing the size
  sMfltRegState core_regs = {0};
  sMemfaultCoredumpSaveInfo save_info = {
    .regs = &core_regs,
    .regs_size = sizeof(core_regs),
    .trace_reason = kMfltRebootReason_UnknownError,
  };

  sCoredumpCrashInfo info = {
    // we'll just pass the current stack pointer, value shouldn't matter
    .stack_address = (void *)&core_regs,
    .trace_reason = save_info.trace_reason,
    .exception_reg_state = NULL,
  };
  save_info.regions = memfault_platform_coredump_get_regions(&info, &save_info.num_regions);

  return memfault_coredump_get_save_size(&save_info);
}

#if !MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM
MEMFAULT_WEAK
void memfault_platform_fault_handler(MEMFAULT_UNUSED const sMfltRegState *regs,
                                     MEMFAULT_UNUSED eMemfaultRebootReason reason) {}
#endif  // MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM

MEMFAULT_USED
void memfault_fault_handler(const sMfltRegState *regs, eMemfaultRebootReason reason) {
  // From "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition",
  // section 'B2.4.10 Data Abort exception', we could decode these, need to make
  // sure we capture the right registers.
  //
  // IFSR, Instruction Fault Status Register
  //
  // // Data Abort types.
  // enumeration DAbort{
  //     DAbort_AccessFlag,
  //     DAbort_Alignment,
  //     DAbort_Background,
  //     DAbort_Domain,
  //     DAbort_Permission,
  //     DAbort_Translation,
  //     DAbort_SyncExternal,
  //     DAbort_SyncExternalonWalk,
  //     DAbort_SyncParity,
  //     DAbort_SyncParityonWalk,
  //     DAbort_AsyncParity,
  //     DAbort_AsyncExternal,
  //     DAbort_SyncWatchpoint,
  //     DAbort_AsyncWatchpoint,
  //     DAbort_TLBConflict,
  //     DAbort_Lockdown,
  //     DAbort_Coproc,
  //     DAbort_ICacheMaint,
  // };

  sMfltRegState core_regs = {
    .r0 = regs->r0,
    .r1 = regs->r1,
    .r2 = regs->r2,
    .r3 = regs->r3,
    .r4 = regs->r4,
    .r5 = regs->r5,
    .r6 = regs->r6,
    .r7 = regs->r7,
    .r8 = regs->r8,
    .r9 = regs->r9,
    .r10 = regs->r10,
    .r11 = regs->r11,
    .r12 = regs->r12,
    .sp = regs->sp,
    .lr = regs->lr,
    .pc = regs->pc,
    .cpsr = regs->cpsr,
  };

  memfault_platform_fault_handler(regs, reason);

  if (s_crash_reason == kMfltRebootReason_Unknown) {
    sMfltRebootTrackingRegInfo info = {
      .pc = core_regs.pc,
      .lr = core_regs.lr,
    };
    memfault_reboot_tracking_mark_reset_imminent(reason, &info);
    s_crash_reason = reason;
  }

  sMemfaultCoredumpSaveInfo save_info = {
    .regs = &core_regs,
    .regs_size = sizeof(core_regs),
    .trace_reason = s_crash_reason,
  };

  sCoredumpCrashInfo info = {
    .stack_address = (void *)core_regs.sp,
    .trace_reason = save_info.trace_reason,
    .exception_reg_state = regs,
  };
  save_info.regions = memfault_platform_coredump_get_regions(&info, &save_info.num_regions);

  const bool coredump_saved = memfault_coredump_save(&save_info);
  if (coredump_saved) {
    memfault_reboot_tracking_mark_coredump_saved();
  }

  memfault_platform_reboot();
  MEMFAULT_UNREACHABLE;
}

// The fault handling shims below figure out what stack was being used leading
// up to the exception, build the sMfltRegState argument and pass that as well
// as the reboot reason to memfault_fault_handler

#if defined(__GNUC__)

// Processor mode values, used when saving LR and SPSR to the appropriate stack.
// From https://developer.arm.com/documentation/dui0801/a/CHDEDCCD
// Defined as strings for macro concatenation below
#define CPU_MODE_FIQ_STR "0x11"
#define CPU_MODE_IRQ_STR "0x12"
#define CPU_MODE_SUPERVISOR_STR "0x13"
#define CPU_MODE_ABORT_STR "0x17"
#define CPU_MODE_UNDEFINED_STR "0x1b"
#define CPU_MODE_SYSTEM_STR "0x1f"

// We push callee r0-r12, lr, pc, and cpsr onto the stack, then pass the stack
// pointer to the C memfault_fault_handler.
//
// Here's a worked example of the position the registers are saved on the stack:
//
// sp = 128 <- starting example stack pointer value
//
// // First push callee pc (exception handler adjusted lr) + spsr (callee cpsr).
// // using 'srsdb'
// cpsr @ 124
// pc @ 120
//
// // Now push user mode r8-r12, sp, lr, by first switching to user mode
// lr @ 116
// sp @ 112
// r12 @ 108
// r11 @ 104
// r10 @ 100
// r9 @ 96
// r8 @ 92
//
// // Now switch back to exception mode, save r0-r7 to the stack
// r7 @ 88
// r6 @ 84
// r5 @ 80
// r4 @ 76
// r3 @ 72
// r2 @ 68
// r1 @ 64
// r0 @ 60 <- this is the struct address we pass to memfault_fault_handler

#define MEMFAULT_FAULT_HANDLING_ASM(_mode_string, _reason) \
  __asm volatile( \
    /* assemble in ARM mode so we can copy sp with a stm instruction */ \
    ".arm \n" \
    /* save callee pc (exception handler adjusted lr) + spsr (callee cpsr) of the current mode */ \
    "srsdb sp!, #" _mode_string " \n" \
    /* push user mode regs at point of fault, including sp + lr */ \
    "mov r8, r1 \n" \
    "mov r1, sp \n" \
    /* Save SPSR and mask mode bits */ \
    "mrs r9, spsr \n" \
    "and r9, #0x1f \n" \
    /* Check each applicable mode and set current mode on a match */ \
    "cmp r9, #" CPU_MODE_IRQ_STR " \n" \
    "bne fiq_mode_%= \n" \
    "cps #" CPU_MODE_IRQ_STR " \n" \
    "b store_regs_%= \n" \
    "fiq_mode_%=: \n" \
    "cmp r9, #" CPU_MODE_FIQ_STR " \n" \
    "bne supervisor_mode_%= \n" \
    "cps #" CPU_MODE_FIQ_STR " \n" \
    "b store_regs_%= \n" \
    "supervisor_mode_%=: \n" \
    "cmp r9, #" CPU_MODE_SUPERVISOR_STR " \n" \
    "bne system_mode_%= \n" \
    "cps #" CPU_MODE_SUPERVISOR_STR " \n" \
    "b store_regs_%= \n" \
    /* Fall back to system mode if no match */ \
    "system_mode_%=: \n" \
    "cps #" CPU_MODE_SYSTEM_STR " \n" \
    "store_regs_%=: \n" \
    "stmfd r1!, {r8-r12, sp, lr} \n"\
    "cps #" _mode_string" \n" \
    /* save active registers in exception frame */ \
    "mov sp, r1 \n" \
    "mov r1, r8 \n" \
    "stmfd sp!, {r0-r7} \n" \
    /* load the pushed frame and reason into r0+r1 for the handler args */ \
    "mov r0, sp \n" \
    "ldr r1, =%c0 \n" \
    "b memfault_fault_handler \n" \
    : \
    : "i"((uint16_t)_reason))

// From the "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition",
// Table B1-7, "Offsets applied to Link value for exceptions taken to PL1 modes"

// |                          |Offset, for        |
// |                          |processor state of:|
// |Exception                 |ARM |Thumb|Jazelle |
// |Undefined Instruction     |+4  |+2   |-b      |
// |Supervisor Call           |None|None |-c      |
// |Secure Monitor Call       |None|None |-c      |
// |Prefetch Abort            |+4  |+4   |+4      |
// |Data Abort                |+8  |+8   |+8      |
// |Virtual Abort             |+8  |+8   |+8      |
// |IRQ or FIQ                |+4  |+4   |+4      |
// |Virtual IRQ or Virtual FIQ|+4  |+4   |+4      |

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_UNDEFINED_INSTRUCTION(void) {
  __asm volatile(
    // for undefined instruction fault, the PC is offset by 2 if in Thumb
    // mode, and 4 if in ARM mode. assemble in ARM mode to avoid ITE block.
    ".arm \n" \
    "push {r0} \n"
    "mrs r0, spsr \n"
    "tst r0, #0x20 \n"  // thumb execution state bit
    "subeq lr, #4 \n"   // ARM mode (!thumb_bit)
    "subne lr, #2 \n"   // Thumb mode
    "pop {r0} \n");

  MEMFAULT_FAULT_HANDLING_ASM(CPU_MODE_UNDEFINED_STR, kMfltRebootReason_UsageFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_DATA_ABORT(void) {
  // data abort applies a 8 byte offset to PC regardless of instruction mode
  __asm volatile("sub lr, #8");
  MEMFAULT_FAULT_HANDLING_ASM(CPU_MODE_ABORT_STR, kMfltRebootReason_MemFault);
}

MEMFAULT_NAKED_FUNC
void MEMFAULT_EXC_HANDLER_PREFETCH_ABORT(void) {
  // prefetch abort applies a 4 byte offset to PC regardless of instruction mode
  __asm volatile("sub lr, #4");
  MEMFAULT_FAULT_HANDLING_ASM(CPU_MODE_ABORT_STR, kMfltRebootReason_BusFault);
}

// TODO, watchdog interrupt wrapper
// MEMFAULT_NAKED_FUNC
// void MEMFAULT_EXC_HANDLER_WATCHDOG(void) {
//   MEMFAULT_FAULT_HANDLING_ASM(kMfltRebootReason_SoftwareWatchdog);
// }

#elif defined(__TI_ARM__)
// Currently only support GCC. See 72bca31346146ad0ff51b02d777d19efb28d6bbe for
// partial TI-ARM compiler support, if needed in the future.
#error "TI ARM compiler is not supported"

#else
  #error "New compiler to add support for!"
#endif

// The ARM architecture has a reserved instruction that is "Permanently
// Undefined" and always generates an Undefined Instruction exception causing an
// ARM fault handler to be invoked.
//
// We use this instruction to "trap" into the fault handler logic. We use 'M'
// (77) as the immediate value for easy disambiguation from any other udf
// invocations in a system.

#if defined(__GNUC__)
  #define MEMFAULT_ASSERT_TRAP() __asm volatile("udf #77")
#else
  #error "Unsupported compiler"
#endif

static void prv_fault_handling_assert(void *pc, void *lr, eMemfaultRebootReason reason) {
  // Only set the crash reason if it's unset, in case we are in a nested assert
  if (s_crash_reason == kMfltRebootReason_Unknown) {
    sMfltRebootTrackingRegInfo info = {
      .pc = (uint32_t)pc,
      .lr = (uint32_t)lr,
    };
    s_crash_reason = reason;
    memfault_reboot_tracking_mark_reset_imminent(s_crash_reason, &info);
  }

  #if MEMFAULT_ASSERT_HALT_IF_DEBUGGING_ENABLED
  memfault_platform_halt_if_debugging();
  #endif

  MEMFAULT_ASSERT_TRAP();

  // We just trap'd into the fault handler logic so it should never be possible
  // to get here but if we do the best thing that can be done is rebooting the
  // system to recover it.
  memfault_platform_reboot();
}

// Note: These functions are annotated as "noreturn" which can be useful for
// static analysis. However, this can also lead to compiler optimizations that
// make recovering local variables difficult (such as ignoring ABI requirements
// to preserve callee-saved registers)
MEMFAULT_NO_OPT
void memfault_fault_handling_assert(void *pc, void *lr) {
  prv_fault_handling_assert(pc, lr, kMfltRebootReason_Assert);

  #if (defined(__clang__) && defined(__ti__))
    //! tiarmclang does not respect the no optimization request and will
    //! strip the pushing callee saved registers making it impossible to recover
    //! an accurate backtrace so we skip over providing the unreachable hint.
  #else
  MEMFAULT_UNREACHABLE;
  #endif
}
MEMFAULT_NO_OPT
void memfault_fault_handling_assert_extra(void *pc, void *lr, sMemfaultAssertInfo *extra_info) {
  prv_fault_handling_assert(pc, lr, extra_info->assert_reason);

  #if (defined(__clang__) && defined(__ti__))
    //! See comment in memfault_fault_handling_assert for more context
  #else
  MEMFAULT_UNREACHABLE;
  #endif
}

#endif  // MEMFAULT_COMPILER_ARM_V7_A_R
