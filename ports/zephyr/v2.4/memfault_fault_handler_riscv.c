//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//!

#include MEMFAULT_ZEPHYR_INCLUDE(arch/cpu.h)
#include MEMFAULT_ZEPHYR_INCLUDE(fatal.h)
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_ctrl.h)
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

// Intercept zephyr/kernel/fatal.c:z_fatal_error(). Note that the signature
// changed in zephyr 3.7.
#if MEMFAULT_ZEPHYR_VERSION_GT(3, 6)
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

#if MEMFAULT_ZEPHYR_VERSION_GT(3, 6)
void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf)
#else
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf)
#endif
{
  // TODO log panic may not be working correctly in fault handler context :(
  // // flush logs prior to capturing coredump & rebooting
  // LOG_PANIC();

  sMfltRegState reg = {
    .mepc = esf->mepc, /* machine exception program counter */
    .ra = esf->ra,
#ifdef CONFIG_USERSPACE
    .sp = esf->sp, /* preserved (user or kernel) stack pointer */
#else
    .sp = esf->s0, /* sp is stashed here in the Zephyr fault shim */
#endif
    // .gp = ?,
    // .tp = ?,
    .t = {
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
    .s = {
      esf->s0, /* callee-saved s0 */
    },
    .a = {
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

MEMFAULT_WEAK MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  // TODO this is not working correctly on the esp32c3 :(
  // memfault_platform_halt_if_debugging();

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
