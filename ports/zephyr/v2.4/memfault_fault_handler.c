//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
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

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);
void __real_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);
// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                            __typeof__(&__wrap_z_fatal_error)) &&
                 __builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                              __typeof__(&__real_z_fatal_error)),
               "Error: check z_fatal_error function signature");

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
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

  memfault_fault_handler(&reg, kMfltRebootReason_HardFault);

#if MEMFAULT_FAULT_HANDLER_RETURN
  // instead of returning, call the Zephyr fatal error handler. This is done
  // here instead of in memfault_platform_reboot(), because we need to pass the
  // function parameters through
  __real_z_fatal_error(reason, esf);
#endif
}

MEMFAULT_WEAK
MEMFAULT_NORETURN
void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
