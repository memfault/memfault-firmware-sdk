//! @file memfault_platform_port.c

#include "hpy_info.h"
#include "memfault/components.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/core/trace_event.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

MEMFAULT_PUT_IN_SECTION(".noinit")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
    static char serial_num_str[HPY_SERIAL_NUM_STR_LEN];
    hpy_get_serial_num_str(serial_num_str);

  // See https://mflt.io/version-nomenclature for more context
  *info = (sMemfaultDeviceInfo) {
     // An ID that uniquely identifies the device in your fleet
     // (i.e serial number, mac addr, chip id, etc)
    .device_serial = serial_num_str,
     // A name to represent the firmware running on the MCU.
     // (i.e "ble-fw", "main-fw", or a codename for your project)
    .software_type = HPY_FW_BUILD_VARIANT_STR,
     // The version of the "software_type" currently running.
     // "software_type" + "software_version" must uniquely represent
     // a single binary
    .software_version = SW_VERSION_STR,
     // The revision of hardware for the device. This value must remain
     // the same for a unique device.
     // (i.e evt, dvt, pvt, or rev1, rev2, etc)
    .hardware_version = HPY_HW_REVISION_STR,
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
   // TODO: Perform any final system cleanup and issue a software reset
   // (i.e NVIC_SystemReset())
   while (1) { } // unreachable
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  // TODO: If device does not track time, stub can be left as is
  //
  // If the device tracks real time, update 'unix_timestamp_secs' with seconds since epoch
  // _and_ change the return value to true. This will cause events logged by the SDK to be
  // timestamped
  *time = (sMemfaultCurrentTime) {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 0
    },
  };
  return false;
}

//! This function _must_ be called by your main() routine prior
//! to starting an RTOS or baremetal loop.
int memfault_platform_boot(void) {
  memfault_freertos_port_boot();

  /* Collect reboot reason */
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);

  memfault_build_info_dump();

  static uint8_t s_event_storage[1024];
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

void test_trace(void)
{
    MEMFAULT_TRACE_EVENT_WITH_LOG(critical_error, "A test error trace!");
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
      break;
  }

  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("[%s] MFLT: %s\n", lvl_str, log_buf);
}
