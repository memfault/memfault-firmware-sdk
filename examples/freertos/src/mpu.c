//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/components.h"

void mpu_init(void) {
  // The below defines are usually provided by the CMSIS headers for the target.
  // They're defined in-line here to avoid needing to bring in a separate SDK.

#define __IM volatile const /*! Defines 'read only' structure member permissions */
#define __IOM volatile      /*! Defines 'read / write' structure member permissions */
  typedef struct {
    __IM uint32_t TYPE;  /*!< Offset: 0x000 (R/ )  MPU Type Register */
    __IOM uint32_t CTRL; /*!< Offset: 0x004 (R/W)  MPU Control Register */
    __IOM uint32_t RNR;  /*!< Offset: 0x008 (R/W)  MPU Region RNRber Register */
    __IOM uint32_t RBAR; /*!< Offset: 0x00C (R/W)  MPU Region Base Address Register */
    __IOM uint32_t RASR; /*!< Offset: 0x010 (R/W)  MPU Region Attribute and Size Register */
  } MPU_Type;
#define MPU ((MPU_Type *)0xe000ed90) /*!< Memory Protection Unit */

  // clang-format off
#define MPU_RBAR_ADDR_Pos                   8U                                            /*!< MPU RBAR: ADDR Position */
#define MPU_RBAR_ADDR_Msk                  (0xFFFFFFUL << MPU_RBAR_ADDR_Pos)              /*!< MPU RBAR: ADDR Mask */

#define MPU_RBAR_VALID_Pos                  4U                                            /*!< MPU RBAR: VALID Position */
#define MPU_RBAR_VALID_Msk                 (1UL << MPU_RBAR_VALID_Pos)                    /*!< MPU RBAR: VALID Mask */

#define MPU_RASR_ENABLE_Pos                 0U                                            /*!< MPU RASR: Region enable bit Position */
#define MPU_RASR_ENABLE_Msk                (1UL /*<< MPU_RASR_ENABLE_Pos*/)               /*!< MPU RASR: Region enable bit Disable Mask */

#define MPU_CTRL_PRIVDEFENA_Pos             2U                                            /*!< MPU CTRL: PRIVDEFENA Position */
#define MPU_CTRL_PRIVDEFENA_Msk            (1UL << MPU_CTRL_PRIVDEFENA_Pos)               /*!< MPU CTRL: PRIVDEFENA Mask */

#define MPU_CTRL_ENABLE_Pos                 0U                                            /*!< MPU CTRL: ENABLE Position */
#define MPU_CTRL_ENABLE_Msk                (1UL /*<< MPU_CTRL_ENABLE_Pos*/)               /*!< MPU CTRL: ENABLE Mask */

#define MPU_RASR_XN_Pos                    28U                                            /*!< MPU RASR: ATTRS.XN Position */
#define MPU_RASR_XN_Msk                    (1UL << MPU_RASR_XN_Pos)                       /*!< MPU RASR: ATTRS.XN Mask */

#define MPU_RASR_SIZE_Pos                   1U                                            /*!< MPU RASR: Region Size Field Position */

#define MPU_RASR_AP_RO (0b110 << 24)  // read-only / read-only
#define MPU_RASR_AP_NO_ACCESS (0b0 << 24)  // no access / no access
  // clang-format on

  const uint32_t dregion = (MPU->TYPE >> 8) & 0xff;
  MEMFAULT_LOG_INFO("MPU_TYPE.DREGION: %lx\n", dregion);

  // If the MPU is not present, there is nothing to do
  if (dregion == 0) {
    MEMFAULT_LOG_INFO("MPU not available\n");
    return;
  }

// Enable 2 MPU regions, to demonstrate Memfault MPU decoding.
//
// The regions are applied to unused memory, to avoid affecting the application.
// 1. Region 0: 0x20100000-0x20100000, read-only, no subregions, no execute never
// 2. Region 1: 0x20100000-0x20100020, no access
//
// The region locations are hard-coded, but work for the an385 + an386 boards.
// Other boards will need to adjust the region locations.
//
// NOTE: These regions are intentionally set up to overlap, to showcase Memfault's
// MPU analyzer feature for coredumps.

// Region 0
#define MPU_RASR_SIZE_128KB (16 << MPU_RASR_SIZE_Pos)  // 2^(SIZE+1) = 2^(16+1) = 2^17 = 128KB
  MPU->RBAR = (0x20100000 & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | 0;
  MPU->RASR = (MPU_RASR_ENABLE_Msk | MPU_RASR_SIZE_128KB | MPU_RASR_AP_RO | MPU_RASR_XN_Msk);

// Region 1
#define MPU_RASR_SIZE_32B (4 << MPU_RASR_SIZE_Pos)  // 2^(SIZE+1) = 2^(4+1) = 2^5 = 32B
  MPU->RBAR = (0x20100000 & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | 1;
  MPU->RASR = (MPU_RASR_ENABLE_Msk | MPU_RASR_SIZE_32B | MPU_RASR_AP_NO_ACCESS);

  // Enable the MPU
  MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
}
