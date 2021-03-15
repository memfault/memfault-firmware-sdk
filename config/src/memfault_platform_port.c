//! @file memfault_platform_port.c

#include "hpy_info.h"

#include "memfault/config.h"
#include "memfault/components.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer.h"
#include "memfault/core/trace_event.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

static bool prv_try_send_memfault_data(void);

__RETAINED_UNINIT MEMFAULT_ALIGNED(8)
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
    static uint8_t serial_num_str[HPY_SERIAL_NUM_STR_LEN] = {0};

    // Only initialize this once
    if(0 == serial_num_str[0])
        hpy_get_bd_addr(serial_num_str);

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
   while (prv_try_send_memfault_data()) { }
   NVIC_SystemReset();
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
  //memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);

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

void test_trace(void)
{
    MEMFAULT_TRACE_EVENT_WITH_LOG(critical_error, "A test error trace!");
    while (prv_try_send_memfault_data()) { }
}

void test_memfault(void)
{
    struct MemfaultDeviceInfo info = {0};
    memfault_platform_get_device_info(&info);
    MEMFAULT_LOG_INFO("S/N: %s", info.device_serial ? info.device_serial : "<NULL>");
    MEMFAULT_LOG_INFO("SW type: %s", info.software_type ? info.software_type : "<NULL>");
    MEMFAULT_LOG_INFO("SW version: %s", info.software_version ? info.software_version : "<NULL>");
    MEMFAULT_LOG_INFO("HW version: %s", info.hardware_version ? info.hardware_version : "<NULL>");

    // Note: Coredump saving runs from an ISR prior to reboot so should
    // be safe to call with interrupts disabled.
    GLOBAL_INT_DISABLE();
    memfault_coredump_storage_debug_test_begin();
    GLOBAL_INT_RESTORE();

    memfault_coredump_storage_debug_test_finish();
    memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
    MEMFAULT_ASSERT(0);
    void (*bad_func)(void) = (void *)0xEEEEDEAD;
    bad_func();
}

// Note: We mark the function as weak so an end user can override this with a real implementation
// and we disable optimizations so the parameters don't get stripped away
MEMFAULT_NO_OPT
MEMFAULT_WEAK
void user_transport_send_chunk_data(MEMFAULT_UNUSED void *chunk_data,
                                    MEMFAULT_UNUSED size_t chunk_data_len) {
}

static bool prv_try_send_memfault_data(void) {
  // buffer to copy chunk data into
  uint8_t buf[MEMFAULT_DEMO_CLI_USER_CHUNK_SIZE];
  size_t buf_len = sizeof(buf);
  bool data_available = memfault_packetizer_get_chunk(buf, &buf_len);
  if (!data_available ) {
    return false; // no more data to send
  }
  // send payload collected to chunks/ endpoint
  user_transport_send_chunk_data(buf, buf_len);
  return true;
}
