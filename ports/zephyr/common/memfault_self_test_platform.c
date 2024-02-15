//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/core/self_test.h"
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

void memfault_self_test_platform_delay(uint32_t delay_ms) {
  k_msleep(delay_ms);
}
