//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//!

#include MEMFAULT_ZEPHYR_INCLUDE(arch/cpu.h)
#include MEMFAULT_ZEPHYR_INCLUDE(fatal.h)
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log_ctrl.h)
#include <soc.h>
#include <xtensa_asm2_context.h>

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/panics/arch/xtensa/xtensa.h"
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

  const _xtensa_irq_stack_frame_raw_t *frame = (void *)esf;
  _xtensa_irq_bsa_t *bsa = frame->ptr_to_bsa;

  sMfltRegState reg = {
    .collection_type = kMemfaultEsp32RegCollectionType_ActiveWindow,
    .pc = bsa->pc,
    .ps = bsa->ps,
    .sar = bsa->sar,
    .exccause = bsa->exccause,
#if XCHAL_HAVE_LOOPS
    .lbeg = bsa->lbeg,
    .lend = bsa->lend,
    .lcount = bsa->lcount,
#endif
    // excvaddr is not available in the bsa on zephyr
  };

  reg.a[0] = bsa->a0;
  // a1 is the stack pointer. Reference: Table 4-114 in Xtensa ISA Reference Manual
  // this decode is pulled from zephyr/arch/xtensa/core/vector_handlers.c, xtensa_dump_stack(),
  // which gets the stack pointer by finding the address right after the bsa
  reg.a[1] = (uintptr_t)((char *)bsa + sizeof(*bsa));
  reg.a[2] = bsa->a2;
  reg.a[3] = bsa->a3;

  size_t i = 4;  // start at index 4 since we've already filled in a[0] - a[3]
  size_t reg_blks_left = MEMFAULT_ARRAY_SIZE(frame->blks);
  const size_t num_regs_in_block = sizeof(frame->blks[0]) / sizeof(frame->blks[0].r0);

  // copy in registers from each register block. Ensure that we don't index out of bounds
  // in the reg.a[] array by checking the last expected index used in the loop:
  // (i + (# regs in block - 1)) < regs.a[] array size
  while (reg_blks_left > 0 && ((i + (num_regs_in_block - 1)) < MEMFAULT_ARRAY_SIZE(reg.a))) {
    reg.a[i++] = frame->blks[reg_blks_left - 1].r0;
    reg.a[i++] = frame->blks[reg_blks_left - 1].r1;
    reg.a[i++] = frame->blks[reg_blks_left - 1].r2;
    reg.a[i++] = frame->blks[reg_blks_left - 1].r3;
    reg_blks_left--;
  }

  memfault_fault_handler(&reg, kMfltRebootReason_HardFault);

  // TODO: calling __real_z_fatal_error() currently results in a infinite fault loop, so the
  // device isn't getting reset properly, or a fault is happening within the fault.
  // Unconditionally reboot the device for now.
  memfault_platform_reboot();
}

MEMFAULT_WEAK MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  // TODO this is not working correctly on the esp32s3
  // memfault_platform_halt_if_debugging();

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
