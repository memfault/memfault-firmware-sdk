#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Handlers for faults & exceptions that are included in the Memfault SDK.

#include <inttypes.h>

#include "memfault/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __arm__
//! Hard Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
MEMFAULT_NAKED_FUNC void HardFault_Handler(void);

//! Memory Management handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
MEMFAULT_NAKED_FUNC void MemoryManagement_Handler(void);

//! Bus Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
MEMFAULT_NAKED_FUNC void BusFault_Handler(void);

//! Usage Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
MEMFAULT_NAKED_FUNC void UsageFault_Handler(void);

//! Non-Maskable Interrupt handler for ARM processors. The handler will capture fault
//! information and PC/LR addresses, trigger a coredump to be captured and finally reboot.
MEMFAULT_NAKED_FUNC void NMI_Handler(void);
#endif

//! Runs the Memfault assert handler.
//!
//! This should be the last function called as part of an a assert. Upon completion
//! it will reboot the system. Normally, this function is used via the
//! MEMFAULT_ASSERT_RECORD and MEMFAULT_ASSERT macros that automatically passes the program
//! counter and return address.
//!
//! @param pc The program counter
//! @param lr The return address
//! @param extra Extra information (reserved for internal use)
//! @see MEMFAULT_ASSERT_RECORD
//! @see MEMFAULT_ASSERT
MEMFAULT_NORETURN
void memfault_fault_handling_assert(void *pc, void *lr, uint32_t extra);

#ifdef __cplusplus
}
#endif
