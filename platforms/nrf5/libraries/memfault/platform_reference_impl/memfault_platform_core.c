//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Reference implementation of the memfault platform header for the NRF52

#include "app_util_platform.h"

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#if !defined(CONFIG_MEMFAULT_EVENT_STORAGE_SIZE)
#define CONFIG_MEMFAULT_EVENT_STORAGE_SIZE 512
#endif

static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];

int memfault_platform_boot(void) {
  memfault_platform_reboot_tracking_boot();
  memfault_build_info_dump();

  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  return 0;
}

MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  memfault_platform_halt_if_debugging();

  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}
