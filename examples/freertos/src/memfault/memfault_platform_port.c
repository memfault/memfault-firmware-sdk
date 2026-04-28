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

#define MEMFAULT_APP_DEBUG 0
#if MEMFAULT_APP_DEBUG
  #define MEMFAULT_APP_PRINTF(...) printf(__VA_ARGS__)
#else
  #define MEMFAULT_APP_PRINTF(...)
#endif

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
  MEMFAULT_APP_PRINTF("Time: %u-%u-%u %u:%u:%u", tm_time->tm_year + 1900, tm_time->tm_mon + 1,
                      tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);

  // If pre-2023, something is wrong
  if ((tm_time->tm_year < 123) || (tm_time->tm_year > 200)) {
    MEMFAULT_APP_PRINTF("Time doesn't make sense: year %u", tm_time->tm_year + 1900);
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

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  sMfltRebootReason stored_reboot_reason;
  int rv = memfault_reboot_tracking_get_reboot_reason(&stored_reboot_reason);
  if (!rv) {
    MEMFAULT_LOG_INFO("Reset Cause Stored: 0x%04x", stored_reboot_reason.prior_stored_reason);
  }
#endif  // MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
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

  #if MEMFAULT_METRICS_BATTERY_ENABLE
  memfault_metrics_battery_boot();
  #endif
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

// ============================================================
// Example: custom coredump memory region for peripheral registers
// ============================================================
//
// This pattern captures peripheral register state at crash time, which is
// invaluable for diagnosing communication failures, DMA overruns, and unexpected
// interrupt conditions.
//
// In production firmware, each struct below would describe a memory-mapped
// peripheral and the region would point at the live hardware address:
//
//   #define UART0_BASE 0x40004000U
//   // In memfault_platform_coredump_get_regions():
//   s_coredump_regions[region_idx] =
//     MEMFAULT_COREDUMP_MEMORY_REGION_INIT((void *)UART0_BASE, sizeof(sExampleUartRegs));

typedef struct {
  uint32_t CR;    // Control:  [0] enable, [15:4] baud-rate divisor, [16] loopback
  uint32_t SR;    // Status:   [0] TX empty, [1] RX data ready, [4] framing err, [5] overrun
  uint32_t BRR;   // Baud Rate Register: configured baud rate in bps
  uint32_t RXCNT; // Cumulative bytes received since last reset
  uint32_t TXCNT; // Cumulative bytes transmitted since last reset
  uint32_t ISR;   // Interrupt Status: sticky error bits, cleared on read in real hardware
} sExampleUartRegs;

typedef struct {
  uint32_t CR;    // Control: [0] enable, [1] CPOL, [2] CPHA, [5:3] clock divider
  uint32_t SR;    // Status:  [0] TX empty, [1] RX full, [2] busy, [3] overrun
  uint32_t DR;    // Data: last byte shifted in/out
  uint32_t RXCNT; // Cumulative bytes received since last reset
  uint32_t TXCNT; // Cumulative bytes transmitted since last reset
} sExampleSpiRegs;

// Sample peripheral register state. Updated during normal operation so that a
// coredump taken after a crash carries meaningful peripheral context.
// Run 'periph_error_crash' from the shell to see this in action.
typedef struct {
  sExampleUartRegs UART0;
  sExampleSpiRegs SPI0;
} sExamplePeriphRegs;

static sExamplePeriphRegs s_example_periph_regs = {
  .UART0 = {
    .CR = (1u << 0),  // UART enabled
    .SR = (1u << 0),  // TX empty (idle at startup)
    .BRR = 115200,
    .RXCNT = 0,
    .TXCNT = 0,
    .ISR = 0,
  },
  .SPI0 = {
    .CR = (1u << 0),  // SPI enabled, mode 0 (CPOL=0, CPHA=0)
    .SR = (1u << 0),  // TX empty (idle at startup)
    .DR = 0,
    .RXCNT = 0,
    .TXCNT = 0,
  },
};

// Simulates errors on both peripherals — call before crashing to produce a
// coredump with non-trivial register state. The 'periph_error_crash' shell
// command does this.
void memfault_example_periph_simulate_error(void) {
  s_example_periph_regs.UART0.TXCNT = 42;
  s_example_periph_regs.UART0.RXCNT = 128;
  s_example_periph_regs.UART0.ISR = (1u << 1);             // overrun sticky bit
  s_example_periph_regs.UART0.SR = (1u << 1) | (1u << 5); // RX data ready + overrun

  s_example_periph_regs.SPI0.TXCNT = 16;
  s_example_periph_regs.SPI0.RXCNT = 15;
  s_example_periph_regs.SPI0.DR = 0xFF;      // last byte during overrun
  s_example_periph_regs.SPI0.SR = (1u << 3); // overrun
}

static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_COREDUMP_MAX_TASK_REGIONS +
                                              2   /* active stack(s) */
                                              + 1 /* _kernel variable */
                                              + 1 /* __memfault_capture_start */
                                              + 2 /* s_task_tcbs + s_task_watermarks */
                                              + 1 /* s_example_periph_regs */
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

  // Capture sample peripheral register state. After decoding the coredump,
  // s_example_periph_regs appears under "Globals & Statics" in the Memfault UI
  // with uart and spi sub-members showing exact register values at crash time.
  s_coredump_regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&s_example_periph_regs, sizeof(s_example_periph_regs));
  region_idx++;

  *num_regions = region_idx;
  return &s_coredump_regions[0];
}
