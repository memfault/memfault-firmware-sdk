//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include "memfault/core/platform/core.h"

// NB: This will require adding CPPUTEST_CPPFLAGS += -DMEMFAULT_NORETURN="" to any Makefile using
// this stub to avoid an error
void memfault_platform_reboot(void) {
  return;
}
