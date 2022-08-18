//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Glue layer between the Memfault SDK and the underlying platform

#include "app_util_platform.h"
#include "nrf.h"

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#if !defined(CONFIG_MEMFAULT_EVENT_STORAGE_SIZE)
#define CONFIG_MEMFAULT_EVENT_STORAGE_SIZE 512
#endif

#if !defined(CONFIG_MEMFAULT_LOGGING_RAM_SIZE)
#define CONFIG_MEMFAULT_LOGGING_RAM_SIZE 512
#endif

static void prv_get_device_serial(char *buf, size_t buf_len) {
  // We will use the 64bit NRF "Device identifier" as the serial number
  const size_t nrf_uid_num_words = 2;

  size_t curr_idx = 0;
  for (size_t i = 0; i < nrf_uid_num_words; i++) {
    uint32_t lsw = NRF_FICR->DEVICEID[i];

    const size_t bytes_per_word =  4;
    for (size_t j = 0; j < bytes_per_word; j++) {

      size_t space_left = buf_len - curr_idx;
      uint8_t val = (lsw >> (j * 8)) & 0xff;
      size_t bytes_written = snprintf(&buf[curr_idx], space_left, "%02X", (int)val);
      if (bytes_written < space_left) {
        curr_idx += bytes_written;
      } else { // we are out of space, return what we got, it's NULL terminated
        return;
      }
    }
  }
}

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  static char s_device_serial[32];
  static bool s_init = false;

  if (!s_init) {
    prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
    s_init = true;
  }

  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = "pca10056",
    .software_version = "1.0.0-dev",
    .software_type = "nrf-main",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  NVIC_SystemReset();
  MEMFAULT_UNREACHABLE;
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  //! RTC is not configured so don't capture time on device
  return false;
}

int memfault_platform_boot(void) {
  // static RAM storage where logs will be stored. Storage can be any size
  // you want but you will want it to be able to hold at least a couple logs.
  static uint8_t s_mflt_log_buf_storage[CONFIG_MEMFAULT_LOGGING_RAM_SIZE];
  memfault_log_boot(s_mflt_log_buf_storage, sizeof(s_mflt_log_buf_storage));

  memfault_build_info_dump();
  memfault_device_info_dump();
  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[CONFIG_MEMFAULT_EVENT_STORAGE_SIZE];
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
