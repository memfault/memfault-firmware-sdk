#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Stub file used to get the NRF5 port to compile in the test environment

#include <stdint.h>

#include "memfault/core/compiler.h"

// Based on https://github.com/NordicSemiconductor/nrfx/blob/master/mdk/nrf52.h
typedef struct {
  uint32_t SIZERAMBLOCKS;
  uint32_t NUMRAMBLOCK;
  struct {
    uint32_t RAM;
  } INFO;
} NRF_FICR_Type;

static const NRF_FICR_Type s_nrf_ficr = {
  .SIZERAMBLOCKS = 0x00000000,
  .NUMRAMBLOCK = 0x00000000,
  .INFO =
    {
      .RAM = 256,
    },
};
static const NRF_FICR_Type* const NRF_FICR = &s_nrf_ficr;

MEMFAULT_USED
static uintptr_t s_get_stack_top(void) {
  uint32_t i;
  uintptr_t retval = (uintptr_t)&i;
  return retval;
}

#define CSTACK s_get_stack_top()
#define STACK_TOP (s_get_stack_top() + 1024)
