//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdio.h>

#include "memfault/components.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/reboot_reason.h"

// Buffer used to store formatted string for output
#define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES (128)

// Reboot tracking storage, must be placed in no-init RAM to keep state after reboot
MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

// Memfault logging storage
static uint8_t s_log_buf_storage[512];

// Use Memfault logging level to filter messages
static eMemfaultPlatformLogLevel s_min_log_level = MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL;
static const char *s_log_prefix = "MFLT";

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "freertos-example",
    .hardware_version = "qemu-mps2-an385",
    .software_type = "qemu-app",
    .software_version = "1.0.0",
  };
}

static const char *prv_level_to_str(eMemfaultPlatformLogLevel level) {
  switch (level) {
    case kMemfaultPlatformLogLevel_Debug:
      return "DEBG";
    case kMemfaultPlatformLogLevel_Info:
      return "INFO";
    case kMemfaultPlatformLogLevel_Warning:
      return "WARN";
    case kMemfaultPlatformLogLevel_Error:
      return "ERRO";
    default:
      return "????";
  }
}

void memfault_platform_log(eMemfaultPlatformLogLevel level, const char *fmt, ...) {
  char log_buf[MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES];

  va_list args;
  va_start(args, fmt);

  if (level >= s_min_log_level) {
    const char *level_name = prv_level_to_str(level);

    vsnprintf(log_buf, sizeof(log_buf), fmt, args);
    printf("%s:[%s] %s\n", s_log_prefix, level_name, log_buf);
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

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = {0};
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

void memfault_metrics_heartbeat_collect_data(void) {
  memfault_metrics_heartbeat_debug_print();
}

int memfault_platform_boot(void) {
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
