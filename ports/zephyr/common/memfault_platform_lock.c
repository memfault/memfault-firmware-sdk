//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Wire up Zephyr locks to the Memfault mutex API

#include "memfault/core/platform/core.h"

#if CONFIG_LEGACY_INCLUDE_PATH
  #include <init.h>
#else
  #include <zephyr/init.h>
  #include <zephyr/kernel.h>
#endif

K_MUTEX_DEFINE(s_memfault_mutex);

void memfault_lock(void) {
  k_mutex_lock(&s_memfault_mutex, K_FOREVER);
}

void memfault_unlock(void) {
  k_mutex_unlock(&s_memfault_mutex);
}
