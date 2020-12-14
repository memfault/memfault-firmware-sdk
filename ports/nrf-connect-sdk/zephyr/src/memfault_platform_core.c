//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!

#include "memfault/core/platform/core.h"

#include <soc.h>
#include <init.h>

#include "memfault/core/build_info.h"
#include "memfault/core/compiler.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/core/trace_event.h"

// On boot-up, log out any information collected as to why the
// reset took place

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

static int prv_init_and_log_reboot(const struct device *dev) {
  memfault_reboot_tracking_boot(s_reboot_tracking, NULL);

  static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_reboot_tracking_collect_reset_info(evt_storage);
  memfault_trace_event_boot(evt_storage);

  memfault_build_info_dump();
  return 0;
}

SYS_INIT(prv_init_and_log_reboot, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
