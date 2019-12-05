//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Glue between Zephyr Fault Handler and Memfault Fault Handler for ARM:
//!  https://github.com/zephyrproject-rtos/zephyr/blob/v1.14-branch/arch/arm/core/fault.c#L786

#include <zephyr.h>

#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/trace_reason_types.h"

void z_SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf) {
  sMfltRegState reg = {
    .exception_frame = (sMfltExceptionFrame *)esf->exception_frame_addr,
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
