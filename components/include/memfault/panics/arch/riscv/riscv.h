#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! ESP32 specific aspects of panic handling

#include <stdint.h>

#include "memfault/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  // A complete dump of all the registers
  kMemfaultEsp32RegCollectionType_Full = 0,
  // A collection of only the active register window
  kMemfaultEsp32RegCollectionType_ActiveWindow = 1,
} eMemfaultEsp32RegCollectionType;

//! Register State collected for ESP32 when a fault occurs
MEMFAULT_PACKED_STRUCT MfltRegState {
  uint32_t collection_type; // eMemfaultEsp32RegCollectionType
  // NOTE: This matches the layout expected for kMemfaultEsp32RegCollectionType_ActiveWindow
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


#ifdef __cplusplus
}
#endif
