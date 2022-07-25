//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/core/platform/core.h"

#include <init.h>
#include <soc.h>

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "zephyr_release_specific_headers.h"

#if !MEMFAULT_ZEPHYR_VERSION_GT(2, 5)

// Note: CONFIG_OPENOCD_SUPPORT was deprecated in Zephyr 2.6 and fully removed in the 3.x line. To
// maintain backward compatibility with older versions of Zephyr we inline the check here.
//
// When support for Zephyr <= 2.5 is removed, we should adopt an approach like the following
//  https://github.com/memfault/memfault-firmware-sdk/pull/26

#if !CONFIG_OPENOCD_SUPPORT
#error "CONFIG_OPENOCD_SUPPORT=y must be added to your prj.conf"
#endif

#endif

#if CONFIG_MEMFAULT_METRICS
#include "memfault/metrics/metrics.h"
#endif

#if CONFIG_MEMFAULT_CACHE_FAULT_REGS
// Zephy's z_arm_fault() function consumes and clears
// the SCB->CFSR register so we must wrap their function
// so we can preserve the pristine fault register values.
void __wrap_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                        _callee_saved_t *callee_regs) {

  memfault_coredump_cache_fault_regs();

  // Now let the Zephyr fault handler complete as normal.
  void __real_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                          _callee_saved_t *callee_regs);
  __real_z_arm_fault(msp, psp, exc_return, callee_regs);
}
#endif

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return k_uptime_get();
}

//! Provide a strong implementation of assert_post_action for Zephyr's built-in
//! __ASSERT() macro.
#if CONFIG_MEMFAULT_CATCH_ZEPHYR_ASSERT
#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
  ARG_UNUSED(file);
  ARG_UNUSED(line);
#endif
  MEMFAULT_ASSERT(0);
}
#endif

// On boot-up, log out any information collected as to why the
// reset took place

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];

#if !CONFIG_MEMFAULT_REBOOT_REASON_GET_CUSTOM
MEMFAULT_WEAK
void memfault_reboot_reason_get(sResetBootupInfo *info) {
  *info = (sResetBootupInfo) {
    .reset_reason = kMfltRebootReason_Unknown,
  };
}
#endif

// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_init_and_log_reboot() {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);

  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);

  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_reboot_tracking_collect_reset_info(evt_storage);
  memfault_trace_event_boot(evt_storage);


#if CONFIG_MEMFAULT_METRICS
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);
#endif

  memfault_build_info_dump();
  return 0;
}

SYS_INIT(prv_init_and_log_reboot,
#if CONFIG_MEMFAULT_INIT_LEVEL_POST_KERNEL
         POST_KERNEL,
#else
         APPLICATION,
#endif
         CONFIG_MEMFAULT_INIT_PRIORITY);
