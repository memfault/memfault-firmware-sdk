//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdbool.h>

#include "memfault/core/self_test.h"
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

void memfault_self_test_platform_delay(uint32_t delay_ms) {
  k_msleep(delay_ms);
}

#if defined(CONFIG_MEMFAULT_SHELL_SELF_TEST_COREDUMP_STORAGE)
static unsigned int s_self_test_lock = 0;

bool memfault_self_test_platform_disable_irqs(void) {
  s_self_test_lock = irq_lock();
  return true;
}

bool memfault_self_test_platform_enable_irqs(void) {
  irq_unlock(s_self_test_lock);
  return true;
}
#endif
