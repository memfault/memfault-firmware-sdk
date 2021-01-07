//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Architecture-specific registers collected collected by Memfault SDK Extra decoding and analysis
//! of these registers is provided from the Memfault cloud

#include <stdint.h>

#include "memfault/core/compiler.h"

#if MEMFAULT_COMPILER_ARM

#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"
#include "memfault/panics/platform/coredump.h"

// By default, enable collection of interrupt state at the time the coredump is collected. User of
// the SDK can disable collection by adding the following compile flag:
// -DMEMFAULT_COLLECT_INTERRUPT_STATE=0
//
// NB: The default Interrupt collection configuration requires ~150 bytes of coredump storage space
// to save.
#ifndef MEMFAULT_COLLECT_INTERRUPT_STATE
#define MEMFAULT_COLLECT_INTERRUPT_STATE 1
#endif

// ARMv7-M supports at least 32 external interrupts in the NVIC and a maximum of 496.
//
// By default, only collect the minimum set. For a typical application this will generally cover
// all the interrupts in use.
//
// If there are more interrupts implemented than analyzed a note will appear in the "ISR Analysis"
// tab for the Issue analyzed in the Memfault UI about the appropriate setting needed.
//
// NB: For each additional 32 NVIC interrupts analyzed, 40 extra bytes are needed for coredump
// storage.
#ifndef MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT
#define MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT 32
#endif

MEMFAULT_STATIC_ASSERT(((MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT) % 32 == 0)
                       || ((MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT) == 496),
                       "Must be a multiple of 32 or 496");
MEMFAULT_STATIC_ASSERT((MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT) <= 512, "Exceeded max possible size");

// Subset of ARMv7-M "System Control and ID blocks" related to fault status
typedef MEMFAULT_PACKED_STRUCT {
  uint32_t SHCSR;
  uint32_t CFSR;
  uint32_t HFSR;
  uint32_t DFSR;
  uint32_t MMFAR;
  uint32_t BFAR;
  uint32_t AFSR;
} sMfltFaultRegs;

typedef MEMFAULT_PACKED_STRUCT {
  uint32_t ICTR;
  uint32_t ACTLR;
  uint32_t rsvd;
  uint32_t SYST_CSR;
} sMfltControlRegs;

typedef MEMFAULT_PACKED_STRUCT {
  uint32_t ICSR;
  uint32_t VTOR;
} sMfltIntControlRegs;

typedef MEMFAULT_PACKED_STRUCT {
  uint32_t SHPR1;
  uint32_t SHPR2;
  uint32_t SHPR3;
} sMfltSysHandlerPriorityRegs;

typedef MEMFAULT_PACKED_STRUCT {
  uint32_t DEMCR;
} sMfltDebugExcMonCtrlReg;

typedef MEMFAULT_PACKED_STRUCT {
  // representation for NVIC ISER, ISPR, and IABR ...
  // A single bit encodes whether or not the interrupt is enabled, pending, active, respectively.
  uint32_t IxxR[(MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT + 31) / 32];
} sMfltNvicIserIsprIabr;

typedef MEMFAULT_PACKED_STRUCT {
  // 8 bits are used to encode the priority so 4 interrupts are covered by each register
  // NB: unimplemented priority levels read back as 0
  uint32_t IPR[MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT / 4];
} sMfltNvicIpr;

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  static sMfltCoredumpRegion s_coredump_regions[] = {
#if !MEMFAULT_COLLECT_INTERRUPT_STATE
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000ED24,
      .region_size = sizeof(sMfltFaultRegs)
    },
#else
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000ED18,
      .region_size = sizeof(sMfltSysHandlerPriorityRegs) + sizeof(sMfltFaultRegs)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E004,
      .region_size = sizeof(sMfltControlRegs)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000ED04,
      .region_size = sizeof(sMfltIntControlRegs)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000EDFC,
      .region_size = sizeof(sMfltDebugExcMonCtrlReg)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E100,
      .region_size = sizeof(sMfltNvicIserIsprIabr)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E200,
      .region_size = sizeof(sMfltNvicIserIsprIabr)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E300,
      .region_size = sizeof(sMfltNvicIserIsprIabr)
    },
    {
      .type = kMfltCoredumpRegionType_MemoryWordAccessOnly,
      .region_start = (void *)0xE000E400,
      .region_size = sizeof(sMfltNvicIpr)
    },
#endif /* MEMFAULT_COLLECT_INTERRUPT_STATE */
  };

  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
  return &s_coredump_regions[0];
}

#endif /* MEMFAULT_COMPILER_ARM */
