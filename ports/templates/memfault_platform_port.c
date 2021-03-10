//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#include <stdbool.h>

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  // See https://mflt.io/version-nomenclature for more context
  *info = (sMemfaultDeviceInfo) {
     // An ID that uniquely identifies the device in your fleet
     // (i.e serial number, mac addr, chip id, etc)
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
    .hardware_version = "dvt1",
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
  // TODO: Add init to any platform specific ports here.
  // (This will be done in later steps in the getting started Guide)

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
