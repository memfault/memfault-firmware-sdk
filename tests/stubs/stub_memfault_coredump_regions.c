//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include "memfault/core/compiler.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(MEMFAULT_UNUSED size_t *num_regions) {
  return NULL;
}

const sMfltCoredumpRegion *memfault_coredump_get_sdk_regions(MEMFAULT_UNUSED size_t *num_regions) {
  return NULL;
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  MEMFAULT_UNUSED const sCoredumpCrashInfo *crash_info, MEMFAULT_UNUSED size_t *num_regions) {
  return NULL;
}
