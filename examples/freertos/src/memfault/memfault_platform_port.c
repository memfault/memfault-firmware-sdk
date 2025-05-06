//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdio.h>
#include <time.h>

#include "memfault/components.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/ports/freertos.h"
#include "memfault/ports/freertos_coredump.h"
#include "memfault/ports/reboot_reason.h"

#if !defined(MEMFAULT_EXAMPLE_USE_REAL_TIME)
  #define MEMFAULT_EXAMPLE_USE_REAL_TIME 1
#endif

// Buffer used to store formatted string for output
#define MEMFAULT_DEBUG_LOG_BUFFER_SIZE_BYTES \
  (sizeof("2024-11-27T14:19:29Z|123456780 I ") + MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN)

#define MEMFAULT_COREDUMP_MAX_TASK_REGIONS ((MEMFAULT_PLATFORM_MAX_TRACKED_TASKS) * 2)

// Reboot tracking storage, must be placed in no-init RAM to keep state after reboot
MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_info") static uint8_t
  s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

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

//! Route the 'export' command to output via printf, so we don't drop messages
//! from logging in a big burst.
void memfault_data_export_base64_encoded_chunk(const char *base64_chunk) {
  printf("%s\n", base64_chunk);
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time_output) {
#if MEMFAULT_EXAMPLE_USE_REAL_TIME
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
#else
  (void)time_output;
  return false;
#endif
}

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
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

static uint32_t prv_read_psp_reg(void) {
  uint32_t reg_val;
  __asm volatile("mrs %0, psp" : "=r"(reg_val));
  return reg_val;
}

static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_COREDUMP_MAX_TASK_REGIONS +
                                              2   /* active stack(s) */
                                              + 1 /* _kernel variable */
                                              + 1 /* __memfault_capture_start */
                                              + 2 /* s_task_tcbs + s_task_watermarks */
];

extern uint32_t __memfault_capture_bss_end;
extern uint32_t __memfault_capture_bss_start;

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  int region_idx = 0;
  const size_t active_stack_size_to_collect = 512;

  // first, capture the active stack (and ISR if applicable)
  const bool msp_was_active = (crash_info->exception_reg_state->exc_return & (1 << 2)) == 0;

  size_t stack_size_to_collect = memfault_platform_sanitize_address_range(
    crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

  s_coredump_regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(crash_info->stack_address, stack_size_to_collect);
  region_idx++;

  if (msp_was_active) {
    // System crashed in an ISR but the running task state is on PSP so grab that too
    void *psp = (void *)prv_read_psp_reg();

    // Collect a little bit more stack for the PSP since there is an
    // exception frame that will have been stacked on it as well
    const uint32_t extra_stack_bytes = 128;
    stack_size_to_collect = memfault_platform_sanitize_address_range(
      psp, active_stack_size_to_collect + extra_stack_bytes);
    s_coredump_regions[region_idx] =
      MEMFAULT_COREDUMP_MEMORY_REGION_INIT(psp, stack_size_to_collect);
    region_idx++;
  }

  // Scoop up memory regions necessary to perform unwinds of the FreeRTOS tasks
  const size_t memfault_region_size =
    (uint32_t)&__memfault_capture_bss_end - (uint32_t)&__memfault_capture_bss_start;

  s_coredump_regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&__memfault_capture_bss_start, memfault_region_size);
  region_idx++;

#if MEMFAULT_TEST_USE_PORT_TEMPLATE != 1
  region_idx += memfault_freertos_get_task_regions(
    &s_coredump_regions[region_idx], MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);
#endif

  *num_regions = region_idx;
  return &s_coredump_regions[0];
}
