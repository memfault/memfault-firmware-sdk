//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port of Memfault dependency functions to mynewt targets

#include <stdbool.h>
#include <syscfg/syscfg.h>

//! Keep os.h above bsp.h; some bsp definitions require the MYNEWT_VAL definition
#include "os/os.h"

#include "bsp/bsp.h"
#include "hal/hal_bsp.h"
#include "hal/hal_system.h"
#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "sysinit/sysinit.h"

#if MYNEWT_VAL(MEMFAULT_ENABLE)

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo) {
    // Note: serial number will be recovered from route used when posting
    // data to the Memfault cloud and does not need to be populated here
    .device_serial = NULL,

    .software_type = MYNEWT_VAL(MEMFAULT_DEVICE_INFO_SOFTWARE_TYPE),
    .software_version = MYNEWT_VAL(MEMFAULT_DEVICE_INFO_SOFTWARE_VERS),
    .hardware_version = MYNEWT_VAL(MEMFAULT_DEVICE_INFO_HARDWARE_VERS),
  };
}
//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  os_system_reset();
  MEMFAULT_UNREACHABLE;
}

// This will cause events logged by the SDK to be timestamped on the device rather than when they
// arrive on the server
bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
#if MYNEWT_VAL(MEMFAULT_USE_DEVICE_TIMESTAMP) != 0
  struct os_timeval tv;
  const int rc = os_gettimeofday(&tv, NULL);
  if (rc != 0) {
    return false;
  }

  *time = (sMemfaultCurrentTime) {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = tv.tv_sec
    },
  };

  return true;
#else
  return false;
#endif
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  int area_cnt;
  const struct hal_bsp_mem_dump *mem_regions = hal_bsp_core_dump(&area_cnt);

  for (size_t i = 0; i < area_cnt; i++) {
    const uint32_t lower_addr = (uint32_t)mem_regions[i].hbmd_start;
    const uint32_t upper_addr = lower_addr + mem_regions[i].hbmd_size;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

#if MYNEWT_VAL(MEMFAULT_METRICS_COMPONENT_ENABLE) != 0

static struct os_callout s_heartbeat_timer_cb;
static uint32_t s_heartbeat_period_ticks;

static void prv_heartbeat_timer_cb(struct os_event *ev) {
  MemfaultPlatformTimerCallback *cb = (MemfaultPlatformTimerCallback *)ev->ev_arg;
  cb();

  os_callout_reset(&s_heartbeat_timer_cb, s_heartbeat_period_ticks);
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec, MemfaultPlatformTimerCallback callback) {
  os_callout_init(&s_heartbeat_timer_cb, os_eventq_dflt_get(),
                  prv_heartbeat_timer_cb, callback);

  s_heartbeat_period_ticks = period_sec * OS_TICKS_PER_SEC;
  const int rc = os_callout_reset(&s_heartbeat_timer_cb, s_heartbeat_period_ticks);
  return (rc == 0);
}

#endif

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return os_get_uptime_usec() / 1000;
}

int memfault_platform_boot(void) {
  memfault_build_info_dump();
  memfault_device_info_dump();
  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[MYNEWT_VAL(MEMFAULT_EVENT_STORAGE_SIZE)];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

#if MYNEWT_VAL(MEMFAULT_METRICS_COMPONENT_ENABLE) != 0
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);
#endif

  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}


static eMemfaultRebootReason prv_get_reboot_reason(enum hal_reset_reason reset_cause) {
  switch (reset_cause) {
    case HAL_RESET_POR:
      return kMfltRebootReason_PowerOnReset;
    case HAL_RESET_PIN:
      return kMfltRebootReason_PinReset;
    case HAL_RESET_WATCHDOG:
      return kMfltRebootReason_HardwareWatchdog;
    case HAL_RESET_SOFT:
      return kMfltRebootReason_SoftwareReset;

    case HAL_RESET_BROWNOUT:
      return kMfltRebootReason_BrownOutReset;
    case HAL_RESET_REQUESTED:
      return kMfltRebootReason_UserReset;
    case HAL_RESET_SYS_OFF_INT:
      return kMfltRebootReason_DeepSleep;
    case HAL_RESET_DFU:
      return kMfltRebootReason_FirmwareUpdate;
    default:
      return kMfltRebootReason_Unknown;
  }
}

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  const enum hal_reset_reason reset_cause = hal_reset_cause();
  *info = (sResetBootupInfo) {
    // The mynewt hal does not expose the raw mcu register value so
    // leave unpopulated for now
    .reset_reason_reg = 0,
    .reset_reason = prv_get_reboot_reason(reset_cause)
  };
}

static bssnz_t uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

#if MYNEWT_VAL(MEMFAULT_COREDUMP_CB)


static eMemfaultRebootReason s_reboot_reason = kMfltRebootReason_UnknownError;

#if MYNEWT_VAL(MEMFAULT_ASSERT_CB)
void os_assert_cb(void) {
  s_reboot_reason = kMfltRebootReason_Assert;
}
#endif

static eMemfaultRebootReason prv_resolve_reason_from_active_isr(void) {
  // ARM Cortex-M have a standard set of exception numbers used for faults.
  //
  // The bottom byte of the XPSR register tells us which interrupt we are running from.
  // See https://mflt.io/cortex-m-exc-numbering
  uint32_t vect_active = __get_xPSR() & 0xff;
  switch (vect_active) {
    case 2:
      return kMfltRebootReason_Nmi;
    case 3:
      return kMfltRebootReason_HardFault;
    case 4:
      return kMfltRebootReason_MemFault;
    case 5:
      return kMfltRebootReason_BusFault;
    case 6:
      return kMfltRebootReason_UsageFault;
    default:
      return kMfltRebootReason_HardFault;
  }
}

void os_coredump_cb(void *tf) {
  if (s_reboot_reason == kMfltRebootReason_UnknownError) {
    s_reboot_reason = prv_resolve_reason_from_active_isr();
  }

  memfault_fault_handler(tf, s_reboot_reason);
}
#endif /* MYNEWT_VAL(MEMFAULT_COREDUMP_CB) */

#endif /* MYNEWT_VAL(MEMFAULT_ENABLE) */
