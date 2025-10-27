//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//!

// clang-format off
#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/fatal.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/arch/exception.h>
#include <soc.h>

#include "memfault/core/build_info.h"
#include "memfault/core/compiler.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/panics/arch/posix/posix.h"
#include "memfault/panics/coredump.h"
#include "memfault/ports/zephyr/log_panic.h"
#include "memfault/panics/fault_handling.h"

// clang-format on

MEMFAULT_PACKED_STRUCT sMfltElfNote {
  uint32_t namesz;                           //!< Size of the name
  uint32_t descsz;                           //!< Size of the data
  uint32_t type;                             //!< Type of the note
  char namedata[4 + MEMFAULT_BUILD_ID_LEN];  //!< Name and data of the note
};

extern struct sMfltElfNote __start_gnu_build_id_start;
MEMFAULT_WEAK struct sMfltElfNote __start_gnu_build_id_start = {
  .namesz = 4,  // "GNU\0"
  .descsz = MEMFAULT_BUILD_ID_LEN,
  .type = 1,  // kMemfaultBuildIdType_GnuBuildIdSha1
  .namedata = { 'G', 'N', 'U', '\0' },
};

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
  }
}

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()

void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf);
void __real_z_fatal_error(unsigned int reason, const struct arch_esf *esf);
// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                            __typeof__(&__wrap_z_fatal_error)) &&
                 __builtin_types_compatible_p(__typeof__(&z_fatal_error),
                                              __typeof__(&__real_z_fatal_error)),
               "Error: check z_fatal_error function signature");

void __wrap_z_fatal_error(unsigned int reason, const struct arch_esf *esf) {
  // flush logs prior to capturing coredump & rebooting
  MEMFAULT_LOG_PANIC();

  sMfltRegState reg = { 0 };

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
