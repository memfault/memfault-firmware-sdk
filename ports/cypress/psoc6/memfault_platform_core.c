//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Implementation of core dependencies and setup for PSOC6 based products that are using the
//! ModusToolbox SDK

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/ports/freertos.h"

#ifndef MEMFAULT_EVENT_STORAGE_RAM_SIZE
#define MEMFAULT_EVENT_STORAGE_RAM_SIZE 1024
#endif

MEMFAULT_WEAK
void memfault_platform_reboot(void) {
  NVIC_SystemReset();
  while (1) { } // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    {.start_addr = CY_SRAM_BASE, .length = CY_SRAM_SIZE},
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

int memfault_platform_boot(void) {
  memfault_build_info_dump();
  memfault_device_info_dump();

  memfault_freertos_port_boot();
  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[MEMFAULT_EVENT_STORAGE_RAM_SIZE];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
