//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include "memfault/core/compiler.h"
#include "memfault/panics/coredump.h"

bool memfault_coredump_storage_check_size(void) {
  return false;
}

bool memfault_coredump_has_valid_coredump(MEMFAULT_UNUSED size_t *total_size_out) {
  return false;
}
