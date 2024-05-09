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
#include "memfault/panics/arch/xtensa/xtensa.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/fault_handling.h"
#include "memfault/ports/zephyr/version.h"

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
  // TODO log panic may not be working correctly in fault handler context :(
  // // flush logs prior to capturing coredump & rebooting
  // LOG_PANIC();

  sMfltRegState reg = {
    // TODO, fill out
  };

  // Unconditionally reboot for now, until coredump support is fully enabled
  memfault_platform_reboot();
  // memfault_fault_handler(&reg, kMfltRebootReason_HardFault);
}

MEMFAULT_WEAK MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  // TODO this is not working correctly on the esp32s3
  // memfault_platform_halt_if_debugging();

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
