//! @file
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
#include "memfault/panics/arch/riscv/riscv.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"
#include "memfault/ports/zephyr/version.h"

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
  // flush logs prior to capturing coredump & rebooting
  LOG_PANIC();

  sMfltRegState reg = {
    .mepc = esf->mepc, /* machine exception program counter */
    .ra = esf->ra,
#ifdef CONFIG_USERSPACE
    .sp = esf->sp, /* preserved (user or kernel) stack pointer */
#endif
    // .gp = ?,
    // .tp = ?,
    .t =
      {
        esf->t0, /* Caller-saved temporary register */
        esf->t1, /* Caller-saved temporary register */
        esf->t2, /* Caller-saved temporary register */
#if !defined(CONFIG_RISCV_ISA_RV32E)
        esf->t3, /* Caller-saved temporary register */
        esf->t4, /* Caller-saved temporary register */
        esf->t5, /* Caller-saved temporary register */
        esf->t6, /* Caller-saved temporary register */
#endif
      },
    .s =
      {
        esf->s0, /* callee-saved s0 */
      },
    .a =
      {
        esf->a0, /* function argument/return value */
        esf->a1, /* function argument */
        esf->a2, /* function argument */
        esf->a3, /* function argument */
        esf->a4, /* function argument */
        esf->a5, /* function argument */
#if !defined(CONFIG_RISCV_ISA_RV32E)
        esf->a6, /* function argument */
        esf->a7, /* function argument */
#endif
      },
    .mstatus = esf->mstatus,
    // .mtvec = ?,
    // .mcause = ?,
    // .mtval = ?,
    // .mhartid = ?,
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
