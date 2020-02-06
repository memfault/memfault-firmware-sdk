//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for the NRF52

#include "app_util_platform.h"
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/trace_event.h"

#if !defined(CONFIG_MEMFAULT_EVENT_STORAGE_SIZE)
#define CONFIG_MEMFAULT_EVENT_STORAGE_SIZE 100
#endif

//! Sanity check that coredump storage is large enough to capture the regions we want to collect
static void prv_assert_coredump_storage_appropriately_sized(void) {
  sMfltCoredumpStorageInfo storage_info = { 0 };
  memfault_platform_coredump_storage_get_info(&storage_info);
  const size_t size_needed = memfault_coredump_storage_compute_size_required();
  if (size_needed > storage_info.size) {
    MEMFAULT_LOG_ERROR("Coredump storage too small. Got %d B, need %d B",
                       storage_info.size, size_needed);
  }
  MEMFAULT_ASSERT(size_needed <= storage_info.size);
}

int memfault_platform_boot(void) {
  MEMFAULT_LOG_INFO("Memfault demo app starting!");
  prv_assert_coredump_storage_appropriately_sized();

  static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));

  memfault_trace_event_boot(evt_storage);
  return 0;
}

void memfault_platform_halt_if_debugging(void) {
  NRF_BREAKPOINT_COND;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
