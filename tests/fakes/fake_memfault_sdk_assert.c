//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <assert.h>

#include "memfault/core/sdk_assert.h"
#include "memfault/core/compiler.h"

MEMFAULT_NORETURN
void memfault_sdk_assert_func(void) {
  assert(0);
}
