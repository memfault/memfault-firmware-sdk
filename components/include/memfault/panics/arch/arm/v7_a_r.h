#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! ARMv7-R (Cortex-R) specific aspects of panic handling

#include <stdint.h>

#include "memfault/core/compiler.h"
#include "memfault/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Register State collected for ARMv7-R when a fault occurs. Non-arch-specific
//! name for the struct type- this type is used when defining the coredump
//! header layout.
MEMFAULT_PACKED_STRUCT MfltRegState {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t sp;  // R13
  uint32_t lr;  // R14
  uint32_t pc;  // R15
  uint32_t cpsr;
};

//! Called by platform assertion handlers to save info before triggering fault
//! handling
//!
//! @param pc Pointer to address of assertion location
//! @param lr Pointer to return address of assertion location
//! @param reason Reboot reason for the assertion
void memfault_arch_fault_handling_assert(void *pc, void *lr, eMemfaultRebootReason reason);

#ifdef __cplusplus
}
#endif
