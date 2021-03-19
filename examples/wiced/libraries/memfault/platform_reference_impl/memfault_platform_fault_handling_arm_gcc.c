//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handler shims to link the WICED SDK fault handlers to the Memfault ARM exception handlers
//! This allows for crash information to be collected when the system hits a fault.

#include "memfault/core/compiler.h"
#include "memfault/panics/fault_handling.h"

// WICED SDK uses unconventional names for the ARM exception handlers.
// This file provides shims to jump to the implementations that libmemfault provides.
// Note we're using a tiny bit of assembly here -- 'b' (branch without link) --
// instead of doing a C function call, which would/could lead to 'bl' (branch with link)
// and cause the LR to get changed.
//
// This wastes a couple bytes of code space. More efficient would be to use the
// --defsym link option, but it does not seem possible to use this with WICED SDK's
// makefile set up. The GLOBAL_LDFLAGS variables almost makes it possible, but the
// problem is that GLOBAL_LDFLAGS is *also* used to link DCT.elf, which will fail when
// using --defsym to define these handlers...

MEMFAULT_NAKED_FUNC void HardFaultException(void) {
  __asm("b HardFault_Handler");
}

MEMFAULT_NAKED_FUNC void MemManageException(void) {
  __asm("b MemoryManagement_Handler");
}

MEMFAULT_NAKED_FUNC void BusFaultException(void) {
  __asm("b BusFault_Handler");
}

MEMFAULT_NAKED_FUNC void UsageFaultException(void) {
  __asm("b UsageFault_Handler");
}

MEMFAULT_NAKED_FUNC void NMIException(void) {
  __asm("b NMI_Handler");
}
