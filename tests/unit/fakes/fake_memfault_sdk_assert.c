//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <assert.h>

#include "memfault/core/compiler.h"
#include "memfault/core/sdk_assert.h"

MEMFAULT_NORETURN void memfault_sdk_assert_func(void) {
  assert(0);
}
