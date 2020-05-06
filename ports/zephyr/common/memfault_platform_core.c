//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/core/platform/core.h"

#include <soc.h>
#include <init.h>
#include "zephyr_release_specific_headers.h"

#include "memfault/core/compiler.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/trace_event.h"
#include "memfault/panics/reboot_tracking.h"

#if !defined(CONFIG_MEMFAULT_EVENT_STORAGE_SIZE)
#define CONFIG_MEMFAULT_EVENT_STORAGE_SIZE 100
#endif

void memfault_platform_reboot(void) {
  const int reboot_type_unused = 0; // ignored for ARM
  sys_reboot(reboot_type_unused);
  MEMFAULT_UNREACHABLE;
}

// By default, the Zephyr NMI handler is an infinite loop. Instead
// let's register the Memfault Exception Handler
static int prv_install_nmi_handler(struct device *dev) {
  extern void z_NmiHandlerSet(void (*pHandler)(void));
  extern void NMI_Handler(void);
  z_NmiHandlerSet(NMI_Handler);
  return 0;
}

SYS_INIT(prv_install_nmi_handler, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

// On boot-up, log out any information collected as to why the
// reset took place

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

static int prv_init_and_log_reboot(struct device *dev) {
  memfault_reboot_tracking_boot(s_reboot_tracking, NULL);

  static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_reboot_tracking_collect_reset_info(evt_storage);
  memfault_trace_event_boot(evt_storage);
  return 0;
}

SYS_INIT(prv_init_and_log_reboot, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
