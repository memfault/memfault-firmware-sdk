#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Handlers for faults & exceptions that are included in the Memfault SDK.

#include <inttypes.h>

#include "memfault/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MEMFAULT_COMPILER_ARM

// By default, exception handlers use CMSIS naming conventions. By default, the CMSIS library
// provides a weak implementation for each handler that is implemented as an infinite loop. By
// using the same name, the Memfault SDK can override this default behavior to capture crash
// information when a fault handler runs.
//
// However, if needed, each handler can be renamed using the following
// preprocessor defines:

//! Hard Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
#ifndef MEMFAULT_EXC_HANDLER_HARD_FAULT
#  define MEMFAULT_EXC_HANDLER_HARD_FAULT HardFault_Handler
#endif

//! Memory Management handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
#ifndef MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT
#  define MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT MemoryManagement_Handler
#endif

//! Bus Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
#ifndef MEMFAULT_EXC_HANDLER_BUS_FAULT
#  define MEMFAULT_EXC_HANDLER_BUS_FAULT BusFault_Handler
#endif

//! Usage Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
#ifndef MEMFAULT_EXC_HANDLER_USAGE_FAULT
#  define MEMFAULT_EXC_HANDLER_USAGE_FAULT UsageFault_Handler
#endif

//! Non-Maskable Interrupt handler for ARM processors. The handler will capture fault
//! information and PC/LR addresses, trigger a coredump to be captured and finally reboot.
//!
//! The NMI is the highest priority interrupt that can run on ARM devices and as the name implies
//! cannot be disabled. Any fault from the NMI handler will trigger the ARM "lockup condition"
//! which results in a reset.
//!
//! Usually a system will reboot or enter an infinite loop if an NMI interrupt is pended so we
//! recommend overriding the default implementation with the Memfault Handler so you can discover
//! if that ever happens.
//!
//! However, a few RTOSs and vendor SDKs make use of the handler. If you encounter a duplicate
//! symbol name conflict due to this the memfault implementation can be disabled as follows:
//!   CFLAGS += -DMEMFAULT_EXC_HANDLER_NMI=MemfaultNmi_Handler_Disabled
#ifndef MEMFAULT_EXC_HANDLER_NMI
#  define MEMFAULT_EXC_HANDLER_NMI NMI_Handler
#endif

//! (Optional) interrupt handler which can be installed for a watchdog
//!
//! If a Watchdog Peripheral supports an early wakeup interrupt or a timer peripheral
//! has been configured as a "software" watchdog, this function should be used as
//! the interrupt handler.
//!
//! For more ideas about configuring watchdogs in general check out:
//!   https://mflt.io/root-cause-watchdogs
#ifndef MEMFAULT_EXC_HANDLER_WATCHDOG
#  define MEMFAULT_EXC_HANDLER_WATCHDOG MemfaultWatchdog_Handler
#endif

//! Function prototypes for fault handlers
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_NMI(void);
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_HARD_FAULT(void);
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT(void);
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_BUS_FAULT(void);
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_USAGE_FAULT(void);
MEMFAULT_NAKED_FUNC void MEMFAULT_EXC_HANDLER_WATCHDOG(void);
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
