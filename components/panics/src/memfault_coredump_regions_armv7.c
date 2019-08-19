//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! Architecture-specific registers collected collected by Memfault SDK Extra decoding and analysis
//! of these registers is provided from the Memfault Cloud

#include <stdint.h>

#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"

// Subset of ARMv7-M "System Control and ID blocks" related to fault status
typedef struct MEMFAULT_PACKED {
  uint32_t SHCSR;
  uint32_t CFSR;
  uint32_t HFSR;
  uint32_t DFSR;
  uint32_t MMFAR;
  uint32_t BFAR;
  uint32_t AFSR;
} sMfltFaultRegs;

// armv7 fault registers
const static sMfltFaultRegs *s_fault_regs = (void *)0xE000ED24;

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  static sMfltCoredumpRegion s_coredump_regions[1];

  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(s_fault_regs, sizeof(sMfltFaultRegs));

  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
  return &s_coredump_regions[0];
}
