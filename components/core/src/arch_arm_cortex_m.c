//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdint.h>

#include "memfault/core/arch.h"

bool memfault_arch_is_inside_isr(void) {
  // We query the "Interrupt Control State Register" to determine
  // if there is an active Exception Handler
  volatile uint32_t *ICSR = (uint32_t *)0xE000ED04;
  // Bottom byte makes up "VECTACTIVE"
  return ((*ICSR & 0xff) != 0x0);
}
