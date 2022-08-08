//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//!

#include <arch/cpu.h>
#include <fatal.h>
#include <init.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <soc.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"

// By default, the Zephyr NMI handler is an infinite loop. Instead
// let's register the Memfault Exception Handler
//
// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_install_nmi_handler() {
  extern void z_NmiHandlerSet(void (*pHandler)(void));
  z_NmiHandlerSet(MEMFAULT_EXC_HANDLER_NMI);
  return 0;
}

SYS_INIT(prv_install_nmi_handler, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

#if CONFIG_MEMFAULT_ZEPHYR_FATAL_HANDLER
extern void __real_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

// This struct stores the crash info for later passing to __real_z_fatal_error
static struct save_crash_info {
  bool valid;
  unsigned int reason;
  z_arch_esf_t esf;
} s_save_crash_info;

// stash the reason and esf for later use by zephyr unwinder
static void prv_save_crash_info(unsigned int reason, const z_arch_esf_t *esf) {
  s_save_crash_info.valid = true;
  s_save_crash_info.reason = reason;
  s_save_crash_info.esf = *esf;
}
#endif

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
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

#if CONFIG_MEMFAULT_ZEPHYR_FATAL_HANDLER
  prv_save_crash_info(reason, esf);
#endif

  memfault_fault_handler(&reg, kMfltRebootReason_HardFault);
}

#if CONFIG_MEMFAULT_ZEPHYR_FATAL_HANDLER
void memfault_zephyr_z_fatal_error(void) {
  if (s_save_crash_info.valid) {
    unsigned int reason = K_ERR_KERNEL_OOPS;
    z_arch_esf_t *esf = NULL;
    if (s_save_crash_info.valid) {
      reason = s_save_crash_info.reason;
      esf = &s_save_crash_info.esf;
    }

    __real_z_fatal_error(reason, esf);
  }
}
#endif

MEMFAULT_WEAK
MEMFAULT_NORETURN
void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

#if CONFIG_MEMFAULT_ZEPHYR_FATAL_HANDLER
  memfault_zephyr_z_fatal_error();
#endif

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
