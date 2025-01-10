//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdio.h>
#include <time.h>

#include "memfault/components.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/reboot_reason.h"
// Buffer used to store formatted string for output
#define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES \
  (sizeof("2024-11-27T14:19:29Z|123456780 I ") + MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN)

// Reboot tracking storage, must be placed in no-init RAM to keep state after reboot
MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

// Memfault logging storage
static uint8_t s_log_buf_storage[512];

// Use Memfault logging level to filter messages
static eMemfaultPlatformLogLevel s_min_log_level = MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL;

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "freertos-example",
    .hardware_version = BOARD,
    .software_type = "qemu-app",
    .software_version = "1.0.0",
  };
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];

  va_list args;
  va_start(args, fmt);

  if (level >= s_min_log_level) {
    vsnprintf(log_buf, sizeof(log_buf), fmt, args);
    // If needed, additional data could be emitted in the log line (timestamp,
    // etc). Here we'll insert ANSI color codes depending on log level.
    switch (level) {
      case kMemfaultPlatformLogLevel_Debug:
        printf("\033[0;32m");
        break;
      case kMemfaultPlatformLogLevel_Info:
        printf("\033[0;37m");
        break;
      case kMemfaultPlatformLogLevel_Warning:
        printf("\033[0;33m");
        break;
      case kMemfaultPlatformLogLevel_Error:
        printf("\033[0;31m");
        break;
      default:
        break;
    }
    printf("%s", log_buf);
    printf("\033[0m\n");
  }

  va_end(args);
}

void memfault_platform_log_raw(const char *fmt, ...) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];
  va_list args;
  va_start(args, fmt);

  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  printf("%s\n", log_buf);

  va_end(args);
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time_output) {
  // Get time from time.h

  // Debug: print time fields
  time_t time_now = time(NULL);

  struct tm *tm_time = gmtime(&time_now);
  MEMFAULT_LOG_DEBUG("Time: %u-%u-%u %u:%u:%u", tm_time->tm_year + 1900, tm_time->tm_mon + 1,
                     tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);

  // If pre-2023, something is wrong
  if ((tm_time->tm_year < 123) || (tm_time->tm_year > 200)) {
    MEMFAULT_LOG_WARN("Time doesn't make sense: year %u", tm_time->tm_year + 1900);
    return false;
  }

  // load the timestamp and return true for a valid timestamp
  *time_output = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = (uint64_t)time_now,
    },
  };
  return true;
}

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

void memfault_metrics_heartbeat_collect_data(void) {
  memfault_metrics_heartbeat_debug_print();
}

int memfault_platform_boot(void) {
  puts(MEMFAULT_BANNER_COLORIZED);

  memfault_freertos_port_boot();

  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[1024];
  const sMemfaultEventStorageImpl *evt_storage =
    memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

#if defined(MEMFAULT_COMPONENT_metrics_)
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);
#endif

  memfault_log_boot(s_log_buf_storage, MEMFAULT_ARRAY_SIZE(s_log_buf_storage));

  memfault_build_info_dump();
  memfault_device_info_dump();
  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
