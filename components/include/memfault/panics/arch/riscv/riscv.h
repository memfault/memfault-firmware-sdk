#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! RISC-V specific aspects of panic handling

#include <stdint.h>

#include "memfault/core/compiler.h"
#include "memfault/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Register State collected for RISC-V when a fault occurs
MEMFAULT_PACKED_STRUCT MfltRegState {
  uint32_t mepc;
  uint32_t ra;
  uint32_t sp;
  uint32_t gp;
  uint32_t tp;
  uint32_t t[7];
  uint32_t s[12];
  uint32_t a[8];
  uint32_t mstatus;
  uint32_t mtvec;
  uint32_t mcause;
  uint32_t mtval;
  uint32_t mhartid;
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
