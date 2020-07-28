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
static int prv_install_nmi_handler(struct device *dev) {
  extern void z_NmiHandlerSet(void (*pHandler)(void));
  z_NmiHandlerSet(MEMFAULT_EXC_HANDLER_NMI);
  return 0;
}

SYS_INIT(prv_install_nmi_handler, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);


// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
  sMfltRegState reg = {
    .exception_frame = (void *)esf->exception_frame,
    .r4 = esf->callee_regs->r4,
    .r5 = esf->callee_regs->r5,
    .r6 = esf->callee_regs->r6,
    .r7 = esf->callee_regs->r7,
    .r8 = esf->callee_regs->r8,
    .r9 = esf->callee_regs->r9,
    .r10 = esf->callee_regs->r10,
    .r11 = esf->callee_regs->r11,
    .exc_return = esf->callee_regs->exc_return,
  };
  memfault_fault_handler(&reg, kMfltRebootReason_HardFault);

}

MEMFAULT_WEAK
MEMFAULT_NORETURN
void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();
  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
