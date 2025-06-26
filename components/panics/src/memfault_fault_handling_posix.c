//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Fault handling for Posix (i386)

#if defined(__i386__)

  #include "memfault/core/compiler.h"
  #include "memfault/core/platform/core.h"
  #include "memfault/core/reboot_tracking.h"
  #include "memfault/panics/arch/posix/posix.h"
  #include "memfault/panics/assert.h"
  #include "memfault/panics/coredump.h"
  #include "memfault/panics/coredump_impl.h"

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  *num_regions = 0;
  return NULL;
}

static eMemfaultRebootReason s_crash_reason = kMfltRebootReason_Unknown;

static void prv_fault_handling_assert(void *pc, void *lr, eMemfaultRebootReason reason) {
  if (s_crash_reason != kMfltRebootReason_Unknown) {
    // we've already been called once, ignore the second call
    return;
  }
  sMfltRebootTrackingRegInfo info = {
    .pc = (uint32_t)pc,
    .lr = (uint32_t)lr,
  };
  s_crash_reason = reason;
  memfault_reboot_tracking_mark_reset_imminent(s_crash_reason, &info);
}

void memfault_platform_halt_if_debugging(void) {
  // unimplemented
}

bool memfault_arch_is_inside_isr(void) {
  return false;  // unimplemented
}

MEMFAULT_NO_OPT void memfault_fault_handling_assert_extra(void *pc, void *lr,
                                                          sMemfaultAssertInfo *extra_info) {
  prv_fault_handling_assert(pc, lr, extra_info->assert_reason);
  memfault_platform_reboot();

  MEMFAULT_UNREACHABLE;
}

MEMFAULT_NO_OPT void memfault_fault_handling_assert(void *pc, void *lr) {
  prv_fault_handling_assert(pc, lr, kMfltRebootReason_Assert);
  memfault_platform_reboot();

  MEMFAULT_UNREACHABLE;
}

void memfault_fault_handler(const sMfltRegState *regs, eMemfaultRebootReason reason) {
  if (s_crash_reason == kMfltRebootReason_Unknown) {
    // skip LR saving here.
    prv_fault_handling_assert((void *)regs->eip, (void *)0, reason);
  }

  sMemfaultCoredumpSaveInfo save_info = {
    .regs = regs,
    .regs_size = sizeof(*regs),
    .trace_reason = s_crash_reason,
  };

  sCoredumpCrashInfo info = {
    .stack_address = (void *)regs->esp,
    .trace_reason = save_info.trace_reason,
    .exception_reg_state = regs,
  };
  save_info.regions = memfault_platform_coredump_get_regions(&info, &save_info.num_regions);

  const bool coredump_saved = memfault_coredump_save(&save_info);
  if (coredump_saved) {
    memfault_reboot_tracking_mark_coredump_saved();
  }
}

size_t memfault_coredump_storage_compute_size_required(void) {
  // actual values don't matter since we are just computing the size
  sMfltRegState core_regs[MEMFAULT_COREDUMP_CPU_COUNT] = { 0 };
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

#endif /* __i386__ */
