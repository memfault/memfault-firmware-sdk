//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include "memfault/core/platform/core.h"

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  static uint64_t boot_time = 1;
  return boot_time++;
}
