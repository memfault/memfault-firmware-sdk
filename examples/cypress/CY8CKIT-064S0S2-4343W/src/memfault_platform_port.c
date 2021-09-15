//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Glue layer between the Memfault SDK and the underlying platform
//!

#include <stdbool.h>

#include <FreeRTOSConfig.h>
#include <core_cm4.h>
#include <cy_device_headers.h>
#include <cy_retarget_io.h>
#include <cy_syslib.h>

#include "memfault/components.h"
#include "memfault/core/compiler.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault_platform_log_config.h"

#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)

typedef struct {
  uint32_t start_addr;
  size_t length;
} sMemRegions;

sMemRegions s_mcu_mem_regions[] = {
  {.start_addr = 0x08030000, .length = 0xB7000},
};

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  // IMPORTANT: All strings returned in info must be constant
  // or static as they will be used _after_ the function returns
  // See https://mflt.io/version-nomenclature for more context
  *info = (sMemfaultDeviceInfo){
    // An ID that uniquely identifies the device in your fleet
    // (i.e serial number, mac addr, chip id, etc)
    // Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
    .device_serial = "DEMOSERIAL",
    // A name to represent the firmware running on the MCU.
    // (i.e "ble-fw", "main-fw", or a codename for your project)
    .software_type = "app-fw",
    // The version of the "software_type" currently running.
    // "software_type" + "software_version" must uniquely represent
    // a single binary
    .software_version = "1.0.0",
    // The revision of hardware for the device. This value must remain
    // the same for a unique device.
    // (i.e evt, dvt, pvt, or rev1, rev2, etc)
    // Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
    .hardware_version = "dvt1",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  NVIC_SystemReset();
  while (1) {
  }  // unreachable
}

//! If device does not track time, return false, else return true if time is valid
bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  *time = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {.unix_timestamp_secs = 0},
  };
  return false;
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  MEMFAULT_SDK_ASSERT(info != NULL);

  const uint32_t reset_cause = Cy_SysLib_GetResetReason();
  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_LOG_INFO("Reset Reason, GetResetReason=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  if (reset_cause & CY_SYSLIB_RESET_HWWDT) {
    MEMFAULT_PRINT_RESET_INFO(" Watchdog Timer Reset");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_ACT_FAULT) {
    MEMFAULT_PRINT_RESET_INFO(" Fault Logging System Active Reset Request");
    reset_reason = kMfltRebootReason_UnknownError;
  } else if (reset_cause & CY_SYSLIB_RESET_DPSLP_FAULT) {
    MEMFAULT_PRINT_RESET_INFO(" Fault Logging System Deep-Sleep Reset Request");
    reset_reason = kMfltRebootReason_DeepSleep;
  } else if (reset_cause & CY_SYSLIB_RESET_SOFT) {
    MEMFAULT_PRINT_RESET_INFO(" Software Reset");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT0) {
    MEMFAULT_PRINT_RESET_INFO(" MCWDT0 Reset");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT1) {
    MEMFAULT_PRINT_RESET_INFO(" MCWDT1 Reset");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT2) {
    MEMFAULT_PRINT_RESET_INFO(" MCWDT2 Reset");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT3) {
    MEMFAULT_PRINT_RESET_INFO(" MCWDT3 Reset");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_HIB_WAKEUP) {
    MEMFAULT_PRINT_RESET_INFO(" Hibernation Exit Reset");
    reset_reason = kMfltRebootReason_DeepSleep;
  } else {
    MEMFAULT_PRINT_RESET_INFO(" Other Error");
    reset_reason = kMfltRebootReason_Unknown;
  }

  Cy_SysLib_ClearResetReason();

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  const char *lvl_str;
  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      lvl_str = "D";
      break;

    case kMemfaultPlatformLogLevel_Info:
      lvl_str = "I";
      break;

    case kMemfaultPlatformLogLevel_Warning:
      lvl_str = "W";
      break;

    case kMemfaultPlatformLogLevel_Error:
      lvl_str = "E";
      break;

    default:
      lvl_str = "D";
      break;
  }

  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("[%s] MFLT: %s\n", lvl_str, log_buf);
}

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = {0};
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

int memfault_platform_boot(void) {
  memfault_freertos_port_boot();

  memfault_build_info_dump();
  memfault_device_info_dump();
  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[1024];
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
