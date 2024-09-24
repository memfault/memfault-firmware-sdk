//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//!

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(arch/cpu.h)
#include MEMFAULT_ZEPHYR_INCLUDE(fatal.h)
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_ctrl.h)
#include <soc.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"
#include "memfault/ports/ncs/version.h"
#include "memfault/ports/zephyr/version.h"

// Starting in v3.4, the handler set function was renamed and the declaration
// added to a public header
#if MEMFAULT_ZEPHYR_VERSION_GT(3, 3)
  #if MEMFAULT_ZEPHYR_VERSION_GT(3, 4)
    #include <cmsis_core.h>
  #else
    #include MEMFAULT_ZEPHYR_INCLUDE(arch/arm/aarch32/nmi.h)
  #endif
  #define MEMFAULT_ZEPHYR_NMI_HANDLER_SET z_arm_nmi_set_handler
#else
  #define MEMFAULT_ZEPHYR_NMI_HANDLER_SET z_NmiHandlerSet
extern void MEMFAULT_ZEPHYR_NMI_HANDLER_SET(void (*pHandler)(void));
#endif  // MEMFAULT_ZEPHYR_VERSION_GT(3, 3)

// The z_fatal_error() signature changed in Zephyr 3.7.0, during release
// candidate development. NCS uses an intermediate version in NCS v2.7.0, so use
// a strict version check instead of also accepting 3.6.99 as equivalent to
// 3.7.0.

// This header is used on Zephyr 3.7+ to get the exception frame declaration
#if MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 6)
#include <zephyr/arch/exception.h>
#endif
// clang-format on

// By default, the Zephyr NMI handler is an infinite loop. Instead
// let's register the Memfault Exception Handler
//
// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_install_nmi_handler() {
  MEMFAULT_ZEPHYR_NMI_HANDLER_SET(MEMFAULT_EXC_HANDLER_NMI);
  return 0;
}

SYS_INIT(prv_install_nmi_handler, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static eMemfaultRebootReason prv_zephyr_to_memfault_fault_reason(unsigned int reason) {
  // See the lists here for reference:
  // https://github.com/zephyrproject-rtos/zephyr/blob/053347375bc00a490ce08fe2b9d78a65183ce95a/include/zephyr/fatal_types.h#L24
  // https://github.com/zephyrproject-rtos/zephyr/blob/053347375bc00a490ce08fe2b9d78a65183ce95a/include/zephyr/arch/arm/arch.h#L59

  switch (reason) {
    // Generic CPU exception, not covered by other codes
    case K_ERR_CPU_EXCEPTION:
    default:
      return kMfltRebootReason_HardFault;

    // Unhandled hardware interrupt
    case K_ERR_SPURIOUS_IRQ:
      return kMfltRebootReason_Hardware;

    // Faulting context overflowed its stack buffer
    case K_ERR_STACK_CHK_FAIL:
      return kMfltRebootReason_StackOverflow;

    // Moderate severity software error
    case K_ERR_KERNEL_OOPS:
    // High severity software error
    case K_ERR_KERNEL_PANIC:
      return kMfltRebootReason_KernelPanic;

#if MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 2)
    // ARM-specific exceptions were added in Zephyr 3.3.0:
    // https://github.com/zephyrproject-rtos/zephyr/pull/53551
    // nRF-Connect SDK v2.2 + v2.3 point to a fork of Zephyr on 3.2.99
    // development version, but it doesn't have the patch with this change, so
    // we only accept strictly > 3.2.

    // Cortex-M MEMFAULT exceptions
    case K_ERR_ARM_MEM_GENERIC:
    case K_ERR_ARM_MEM_STACKING:
    case K_ERR_ARM_MEM_UNSTACKING:
    case K_ERR_ARM_MEM_DATA_ACCESS:
    case K_ERR_ARM_MEM_INSTRUCTION_ACCESS:
    case K_ERR_ARM_MEM_FP_LAZY_STATE_PRESERVATION:
      return kMfltRebootReason_MemFault;

    // Cortex-M BUSFAULT exceptions
    case K_ERR_ARM_BUS_GENERIC:
    case K_ERR_ARM_BUS_STACKING:
    case K_ERR_ARM_BUS_UNSTACKING:
    case K_ERR_ARM_BUS_PRECISE_DATA_BUS:
    case K_ERR_ARM_BUS_IMPRECISE_DATA_BUS:
    case K_ERR_ARM_BUS_INSTRUCTION_BUS:
    case K_ERR_ARM_BUS_FP_LAZY_STATE_PRESERVATION:
      return kMfltRebootReason_BusFault;

    // Cortex-M USAGEFAULT exceptions
    case K_ERR_ARM_USAGE_GENERIC:
    case K_ERR_ARM_USAGE_DIV_0:
    case K_ERR_ARM_USAGE_UNALIGNED_ACCESS:
    case K_ERR_ARM_USAGE_NO_COPROCESSOR:
    case K_ERR_ARM_USAGE_ILLEGAL_EXC_RETURN:
    case K_ERR_ARM_USAGE_ILLEGAL_EPSR:
    case K_ERR_ARM_USAGE_UNDEFINED_INSTRUCTION:
      return kMfltRebootReason_UsageFault;

    case K_ERR_ARM_USAGE_STACK_OVERFLOW:
      return kMfltRebootReason_StackOverflow;

    // Cortex-M SECURE exceptions
    case K_ERR_ARM_SECURE_GENERIC:
    case K_ERR_ARM_SECURE_ENTRY_POINT:
    case K_ERR_ARM_SECURE_INTEGRITY_SIGNATURE:
    case K_ERR_ARM_SECURE_EXCEPTION_RETURN:
    case K_ERR_ARM_SECURE_ATTRIBUTION_UNIT:
    case K_ERR_ARM_SECURE_TRANSITION:
    case K_ERR_ARM_SECURE_LAZY_STATE_PRESERVATION:
    case K_ERR_ARM_SECURE_LAZY_STATE_ERROR:
      return kMfltRebootReason_SecurityViolation;

      // Zephyr + Cortex A/R is currently unsupported, please contact support@memfault.com for
      // assistance!
      // // Cortex-A/R exceptions
      // K_ERR_ARM_UNDEFINED_INSTRUCTION
      // K_ERR_ARM_ALIGNMENT_FAULT
      // K_ERR_ARM_BACKGROUND_FAULT
      // K_ERR_ARM_PERMISSION_FAULT
      // K_ERR_ARM_SYNC_EXTERNAL_ABORT
      // K_ERR_ARM_ASYNC_EXTERNAL_ABORT
      // K_ERR_ARM_SYNC_PARITY_ERROR
      // K_ERR_ARM_ASYNC_PARITY_ERROR
      // K_ERR_ARM_DEBUG_EVENT
      // K_ERR_ARM_TRANSLATION_FAULT
      // K_ERR_ARM_UNSUPPORTED_EXCLUSIVE_ACCESS_FAULT
#endif  // MEMFAULT_ZEPHYR_VERSION_GT(3, 2)
  }
}

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error(). Note that the signature
// changed in zephyr 3.7.
#if defined(CONFIG_MEMFAULT_NRF_CONNECT_SDK)
  #define MEMFAULT_NEW_ARCH_ESF_STRUCT MEMFAULT_NCS_VERSION_GT(2, 7)
#else
  #define MEMFAULT_NEW_ARCH_ESF_STRUCT MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 6)
#endif

#if MEMFAULT_NEW_ARCH_ESF_STRUCT
void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf);
void __real_z_fatal_error(unsigned int reason, const struct arch_esf *esf);
#else
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);
void __real_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);
#endif
// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                            __typeof__(&__wrap_z_fatal_error)) &&
                 __builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                              __typeof__(&__real_z_fatal_error)),
               "Error: check z_fatal_error function signature");

#if MEMFAULT_NEW_ARCH_ESF_STRUCT
void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf)
#else
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
#endif
{
  // flush logs prior to capturing coredump & rebooting
  LOG_PANIC();

  const struct __extra_esf_info *extra_info = &esf->extra_info;
  const _callee_saved_t *callee_regs = extra_info->callee;

  // Read the "SPSEL" bit where
  //  0 = Main Stack Pointer in use prior to exception
  //  1 = Process Stack Pointer in use prior to exception
  const uint32_t exc_return = extra_info->exc_return;
  const bool msp_was_active = (exc_return & (1 << 2)) == 0;

  sMfltRegState reg = {
    .exception_frame = (void *)(msp_was_active ? extra_info->msp : callee_regs->psp),
    .r4 = callee_regs->v1,
    .r5 = callee_regs->v2,
    .r6 = callee_regs->v3,
    .r7 = callee_regs->v4,
    .r8 = callee_regs->v5,
    .r9 = callee_regs->v6,
    .r10 = callee_regs->v7,
    .r11 = callee_regs->v8,
    .exc_return = exc_return,
  };

  eMemfaultRebootReason fault_reason = prv_zephyr_to_memfault_fault_reason(reason);

  memfault_fault_handler(&reg, fault_reason);

#if MEMFAULT_FAULT_HANDLER_RETURN
  // instead of returning, call the Zephyr fatal error handler. This is done
  // here instead of in memfault_platform_reboot(), because we need to pass the
  // function parameters through
  __real_z_fatal_error(reason, esf);
#endif
}

MEMFAULT_WEAK MEMFAULT_NORETURN void memfault_platform_reboot(void) {
#if MEMFAULT_ASSERT_HALT_IF_DEBUGGING_ENABLED
  memfault_platform_halt_if_debugging();
#endif

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
