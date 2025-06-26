#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Posix (Intel 80386) specific aspects of panic handling

#include <stdint.h>

#include "memfault/core/compiler.h"
#include "memfault/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Register State collected for Intel 80386 when a fault occurs
MEMFAULT_PACKED_STRUCT MfltRegState {
  uint32_t eip;     //!< Instruction pointer
  uint32_t cs;      //!< Code segment
  uint32_t eflags;  //!< Flags register
  uint32_t esp;     //!< Stack pointer
  uint32_t ss;      //!< Stack segment
  uint32_t eax;     //!< Accumulator register
  uint32_t ebx;     //!< Base register
  uint32_t ecx;     //!< Counter register
  uint32_t edx;     //!< Data register
  uint32_t edi;     //!< Destination index register
  uint32_t esi;     //!< Source index register
};

//! Called by platform assertion handlers to save info before triggering fault handling
//!
//! @param pc Pointer to address of assertion location
//! @param lr Pointer to return address of assertion location
//! @param reason Reboot reason for the assertion
void memfault_arch_fault_handling_assert(void *pc, void *lr, eMemfaultRebootReason reason);

#ifdef __cplusplus
}
#endif
