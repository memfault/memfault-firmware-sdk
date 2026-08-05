//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Wire up Zephyr locks to the Memfault mutex API

#include "memfault/core/platform/core.h"
#include "memfault/core/sdk_assert.h"

#if defined(CONFIG_MEMFAULT_PLATFORM_LOCK_SOURCE_MUTEX)
  #include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

K_MUTEX_DEFINE(s_memfault_mutex);

void memfault_lock(void) {
  k_mutex_lock(&s_memfault_mutex, K_FOREVER);
}

void memfault_unlock(void) {
  k_mutex_unlock(&s_memfault_mutex);
}

#endif  // defined(CONFIG_MEMFAULT_PLATFORM_LOCK_SOURCE_MUTEX)

#if defined(CONFIG_MEMFAULT_PLATFORM_LOCK_SOURCE_IRQ_LOCK)
  #include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

static atomic_t s_memfault_lock_nesting;
static unsigned int s_memfault_lock_irq_key;

void memfault_lock(void) {
  // atomic_inc() returns the *previous* value, so a previous value of 0
  // means this call is the outermost lock and must actually take irq_lock()
  if (atomic_inc(&s_memfault_lock_nesting) == 0) {
    s_memfault_lock_irq_key = irq_lock();
  }
}

void memfault_unlock(void) {
  MEMFAULT_SDK_ASSERT(atomic_get(&s_memfault_lock_nesting) > 0);

  // atomic_dec() also returns the *previous* value, so a previous value of 1
  // means the nesting count just dropped to 0 and irq_unlock() must run
  if (atomic_dec(&s_memfault_lock_nesting) == 1) {
    irq_unlock(s_memfault_lock_irq_key);
  }
}
#endif  // defined(CONFIG_MEMFAULT_PLATFORM_LOCK_SOURCE_IRQ_LOCK)
