//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Port of the Memfault SDK to the esp-idf for esp32 devices

#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"

// non-module headers below

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "esp_idf_version.h"
#include "esp_intr_alloc.h"
#include "esp_partition.h"

// The include order below, especially 'soc/soc.h', is significant to support
// the various ESP-IDF versions, where the definitions moved around. It's
// possible it could be tidied up but this configuration does work across the
// supported ESP-IDF versions.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  #include "spi_flash_mmap.h"
#else
  #include "esp_spi_flash.h"
  #include "soc/soc.h"
#endif

#include "esp_task_wdt.h"
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/task_watchdog.h"
#include "memfault/esp_port/coredump.h"
#include "memfault/esp_port/spi_flash.h"
#include "memfault/esp_port/uart.h"
#include "memfault/util/crc16.h"

// Needed for >= v4.4.3 default coredump collection
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)
  #include "memfault/ports/freertos_coredump.h"
#endif

// Factor out issues with Espressif's ESP32 to ESP conversion in sdkconfig
#define COREDUMPS_ENABLED (CONFIG_ESP32_ENABLE_COREDUMP || CONFIG_ESP_COREDUMP_ENABLE)
#define COREDUMP_TO_FLASH_ENABLED \
  (CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH || CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH)

#if !COREDUMPS_ENABLED || !COREDUMP_TO_FLASH_ENABLED
  #error \
    "Memfault SDK integration requires CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH=y sdkconfig setting"
#endif

#define ESP_IDF_COREDUMP_PART_INIT_MAGIC 0x45524f43

// If there is no coredump partition defined or one cannot be defined
// the user can try using an OTA slot instead.
#if CONFIG_MEMFAULT_COREDUMP_USE_OTA_SLOT
  #include "esp_ota_ops.h"
  #define GET_COREDUMP_PARTITION() esp_ota_get_next_update_partition(NULL);
#else
  #define GET_COREDUMP_PARTITION() \
    esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL)
#endif

typedef struct {
  uint32_t magic;
  esp_partition_t partition;
  uint32_t crc;
} sEspIdfCoredumpPartitionInfo;

static sEspIdfCoredumpPartitionInfo s_esp32_coredump_partition_info;
static const uintptr_t esp32_dram_start_addr = SOC_DRAM_LOW;
static const uintptr_t esp32_dram_end_addr = SOC_DRAM_HIGH;

static uint32_t prv_get_partition_info_crc(void) {
  return memfault_crc16_compute(MEMFAULT_CRC16_INITIAL_VALUE, &s_esp32_coredump_partition_info,
                                offsetof(sEspIdfCoredumpPartitionInfo, crc));
}

static const esp_partition_t *prv_get_core_partition(void) {
  if (s_esp32_coredump_partition_info.magic != ESP_IDF_COREDUMP_PART_INIT_MAGIC) {
    return NULL;
  }

  return &s_esp32_coredump_partition_info.partition;
}

// We use two different default coredump collection methods
// due to differences in esp-idf versions. The following helper
// is only used for esp-idf >= 4.4.3
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)
//! Helper function to get bss and common sections of task and timer objects
size_t prv_get_freertos_bss_common(sMfltCoredumpRegion *regions, size_t num_regions) {
  if (regions == NULL || num_regions == 0) {
    return 0;
  }

  size_t region_index = 0;
  extern uint32_t _memfault_timers_bss_start;
  extern uint32_t _memfault_timers_bss_end;
  extern uint32_t _memfault_timers_common_start;
  extern uint32_t _memfault_timers_common_end;
  extern uint32_t _memfault_tasks_bss_start;
  extern uint32_t _memfault_tasks_bss_end;
  extern uint32_t _memfault_tasks_common_start;
  extern uint32_t _memfault_tasks_common_end;

  // ldgen has a bug that does not exclude rules matching multiple input sections at the
  // same time. To work around this, we instead emit a symbol for each section we're attempting
  // to collect. This means 8 symbols (tasks/timers + bss/common). If this is ever fixed we
  // can remove the need to collect 4 separate regions.
  size_t timers_bss_length =
    (uintptr_t)&_memfault_timers_bss_end - (uintptr_t)&_memfault_timers_bss_start;
  size_t timers_common_length =
    (uintptr_t)&_memfault_timers_common_end - (uintptr_t)&_memfault_timers_common_start;
  size_t tasks_bss_length =
    (uintptr_t)&_memfault_tasks_bss_end - (uintptr_t)&_memfault_tasks_bss_start;
  size_t tasks_common_length =
    (uintptr_t)&_memfault_tasks_common_end - (uintptr_t)&_memfault_tasks_common_start;

  regions[region_index++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&_memfault_timers_bss_start, timers_bss_length);
  regions[region_index++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&_memfault_timers_common_start, timers_common_length);
  regions[region_index++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&_memfault_tasks_bss_start, tasks_bss_length);
  regions[region_index++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&_memfault_tasks_common_start, tasks_common_length);

  return region_index;
}
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)

//! Simple implementation to ensure address is in SRAM range.
//!
//! @note The function is intentionally defined as weak so someone can
//! easily override the port defaults by re-defining a non-weak version of
//! the function in another file
MEMFAULT_WEAK size_t memfault_platform_sanitize_address_range(void *start_addr,
                                                              size_t desired_size) {
  const uintptr_t start_addr_int = (uintptr_t)start_addr;
  const uintptr_t end_addr_int = start_addr_int + desired_size;

  if ((start_addr_int < esp32_dram_start_addr) || (start_addr_int > esp32_dram_end_addr)) {
    return 0;
  }

  if (end_addr_int > esp32_dram_end_addr) {
    return esp32_dram_end_addr - start_addr_int;
  }

  return desired_size;
}

#if defined(CONFIG_MEMFAULT_COREDUMP_CAPTURE_TASK_WATCHDOG) && \
  (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4))
static char s_mflt_task_watchdog_data[CONFIG_MEMFAULT_COREDUMP_CAPTURE_TASK_WATCHDOG_SIZE];

static void prv_task_watchdog_msg_handler(void *opaque, const char *msg) {
  const int call_count = *(int *)opaque;

  // increment the call count on every call
  *(int *)opaque = call_count + 1;

  // the first call is a caption, skip
  if (call_count == 0) {
    s_mflt_task_watchdog_data[0] = '\0';
    return;
  }
  // after that, this function is called 3 times per triggered task:
  // msg_handler(opaque, "\n - ");
  // msg_handler(opaque, name);
  // msg_handler(opaque, cpu);
  //
  // we will copy only the name + cpu, in this format:
  //  |"<name>" <cpu>||"<name>" <cpu>...
  if ((call_count - 1) % 3 == 0) {
    // skip the first call, which is a caption
    return;
  }
  // copy the name
  if ((call_count - 1) % 3 == 1) {
    strlcat(s_mflt_task_watchdog_data, "|\"", sizeof(s_mflt_task_watchdog_data));
    strlcat(s_mflt_task_watchdog_data, msg, sizeof(s_mflt_task_watchdog_data));
    strlcat(s_mflt_task_watchdog_data, "\"", sizeof(s_mflt_task_watchdog_data));
    return;
  }
  // copy the cpu
  if ((call_count - 1) % 3 == 2) {
    strlcat(s_mflt_task_watchdog_data, msg, sizeof(s_mflt_task_watchdog_data));
    strlcat(s_mflt_task_watchdog_data, "|", sizeof(s_mflt_task_watchdog_data));
    return;
  }
}

size_t prv_get_task_watchdog_region(sMfltCoredumpRegion *regions, size_t num_regions) {
  if (num_regions == 0) {
    return 0;
  }

  // the TWDT might not be initialized, check it first
  esp_err_t err = esp_task_wdt_status(NULL);
  if (err == ESP_ERR_INVALID_STATE) {
    return 0;
  }

  // run the task wdt printer to extract the data
  // keep track of the number of calls to the msg_handler
  int call_count = 0;
  err = esp_task_wdt_print_triggered_tasks(prv_task_watchdog_msg_handler, &call_count, NULL);

  if (err != ESP_OK) {
    return 0;
  } else {
    // attach the s_mflt_task_watchdog_data as a region
    *regions = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      s_mflt_task_watchdog_data,
      strnlen(s_mflt_task_watchdog_data, sizeof(s_mflt_task_watchdog_data) - 1));
    return 1;
  }
}
#endif  // CONFIG_MEMFAULT_COREDUMP_CAPTURE_TASK_WATCHDOG

//! Collecting active stack, bss, data, and heap.
//!
//! In esp-idf >= 4.4.3, we additionally collect bss and stack regions for
//! FreeRTOS tasks.
const sMfltCoredumpRegion *memfault_esp_port_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_ESP_PORT_NUM_REGIONS];
  int region_idx = 0;

  const size_t stack_size = memfault_platform_sanitize_address_range(
    crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

  // First, capture the active stack
  s_coredump_regions[region_idx++] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(crash_info->stack_address, stack_size);

  // Second, capture the task regions, if esp-idf >= 4.4.3
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)
  region_idx += memfault_freertos_get_task_regions(
    &s_coredump_regions[region_idx], MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

  // Third, capture the FreeRTOS-specific bss regions
  region_idx += prv_get_freertos_bss_common(&s_coredump_regions[region_idx],
                                            MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 3)

// Conditionally capture the Task Watchdog data
#if defined(CONFIG_MEMFAULT_COREDUMP_CAPTURE_TASK_WATCHDOG) && \
  (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 4))
  region_idx += prv_get_task_watchdog_region(&s_coredump_regions[region_idx],
                                             MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);
#endif

  // Next, capture all of .bss + .data that we can fit.
  extern uint32_t _data_start;
  extern uint32_t _data_end;
  extern uint32_t _bss_start;
  extern uint32_t _bss_end;
  extern uint32_t _heap_start;

  s_coredump_regions[region_idx++] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
    &_bss_start, (uint32_t)((uintptr_t)&_bss_end - (uintptr_t)&_bss_start));
  s_coredump_regions[region_idx++] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
    &_data_start, (uint32_t)((uintptr_t)&_data_end - (uintptr_t)&_data_start));
  // Finally, capture as much of the heap as we can fit
  s_coredump_regions[region_idx++] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
    &_heap_start, (uint32_t)(esp32_dram_end_addr - (uintptr_t)&_heap_start));

  *num_regions = region_idx;

  return &s_coredump_regions[0];
}

#if defined(CONFIG_MEMFAULT_COREDUMP_REGIONS_BUILT_IN)
//! Built-in Coredump Regions function
//!
//! @note The function is intentionally defined as weak so someone can
//! easily override the port defaults by re-defining a non-weak version of
//! the function in another file
MEMFAULT_WEAK const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  return memfault_esp_port_coredump_get_regions(crash_info, num_regions);
}
#endif  // CONFIG_MEMFAULT_COREDUMP_REGIONS_BUILT_IN

static uint32_t prv_get_adjusted_size(const esp_partition_t *core_part) {
  return
#if CONFIG_MEMFAULT_COREDUMP_STORAGE_MAX_SIZE > 0
    MEMFAULT_MIN(core_part->size, CONFIG_MEMFAULT_COREDUMP_STORAGE_MAX_SIZE)
#else
    core_part->size
#endif
    // coredump storage capacity is reduced by the write offset
    - (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE);
}

//! Opens partition system on boot to determine where a coredump can be saved
//!
//! @note We override the default implementation using the GNU linkers --wrap feature
//! @note Function invocation is here:
//!   https://github.com/espressif/esp-idf/blob/v4.0/components/esp32/cpu_start.c#L415-L422
void __wrap_esp_core_dump_init(void) {
  const esp_partition_t *const core_part = GET_COREDUMP_PARTITION();

  if (core_part == NULL) {
    MEMFAULT_LOG_ERROR("Coredumps enabled but no partition exists!");
    MEMFAULT_LOG_ERROR("Add \"coredump\" to your partition.csv file");
    return;
  }

  MEMFAULT_LOG_INFO("Coredumps will be saved to '%s' partition", core_part->label);
  MEMFAULT_LOG_INFO("Using entry %p pointing to address 0x%08" PRIX32 " size %" PRIu32
                    " writeoffset %d",
                    core_part, core_part->address, prv_get_adjusted_size(core_part),
                    (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE));

  s_esp32_coredump_partition_info = (sEspIdfCoredumpPartitionInfo){
    .magic = ESP_IDF_COREDUMP_PART_INIT_MAGIC,
    .partition = *core_part,
  };
  s_esp32_coredump_partition_info.crc = prv_get_partition_info_crc();

  // Log an error if there is not enough space to save the information requested
  memfault_coredump_storage_check_size();
}

esp_err_t __wrap_esp_core_dump_image_get(size_t *out_addr, size_t *out_size) {
  if (out_addr == NULL || out_size == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  const esp_partition_t *core_part = prv_get_core_partition();
  if (core_part == NULL) {
    return ESP_FAIL;
  }

  if (!memfault_coredump_has_valid_coredump(out_size)) {
    return ESP_ERR_INVALID_SIZE;
  }

  *out_addr =
    core_part->address + CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE;
  return ESP_OK;
}

const esp_partition_t *prv_validate_and_get_core_partition(void) {
  const uint32_t crc = prv_get_partition_info_crc();
  if (crc != s_esp32_coredump_partition_info.crc) {
    return NULL;
  }

  return prv_get_core_partition();
}

void memfault_platform_coredump_storage_clear(void) {
  const esp_partition_t *core_part = prv_get_core_partition();
  if (core_part == NULL) {
    return;
  }

  const uint32_t invalidate = 0x0;
  if (core_part->size < sizeof(invalidate)) {
    return;
  }
  const uint32_t clear_address =
    core_part->address +
    (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE);
  const esp_err_t err =
    memfault_esp_spi_flash_write(clear_address, &invalidate, sizeof(invalidate));
  if (err != ESP_OK) {
    memfault_platform_log(kMemfaultPlatformLogLevel_Error, "Failed to write data to flash (%d)!",
                          err);
  }
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  // we are about to perform a sequence of operations on coredump storage
  // sanity check that the memory holding the info is populated and not corrupted
  const esp_partition_t *core_part = prv_validate_and_get_core_partition();
  if (core_part == NULL) {
    *info = (sMfltCoredumpStorageInfo){ 0 };
    return;
  }

#if defined(CONFIG_MEMFAULT_COREDUMP_STORAGE_LIMIT_SIZE)
  // confirm MAX_SIZE is aligned to SPI_FLASH_SEC_SIZE
  MEMFAULT_STATIC_ASSERT(
    (CONFIG_MEMFAULT_COREDUMP_STORAGE_MAX_SIZE % SPI_FLASH_SEC_SIZE == 0),
    "Error, check CONFIG_MEMFAULT_COREDUMP_STORAGE_MAX_SIZE value: " MEMFAULT_EXPAND_AND_QUOTE(
      CONFIG_MEMFAULT_COREDUMP_STORAGE_MAX_SIZE));
#endif

  *info = (sMfltCoredumpStorageInfo){
    .size = prv_get_adjusted_size(core_part),
    .sector_size = SPI_FLASH_SEC_SIZE,
  };
}

#if !CONFIG_ESP32_PANIC_SILENT_REBOOT
  #include "esp_private/panic_internal.h"

static void prv_panic_safe_putchar(const char c) {
  panic_print_char(c);
}

#else /* !CONFIG_ESP32_PANIC_SILENT_REBOOT */

static void prv_panic_safe_putchar(char c) { }

#endif /* CONFIG_ESP32_PANIC_SILENT_REBOOT */

static void prv_panic_safe_putstr(const char *str) {
  int idx = 0;
  while (str[idx] != 0) {
    prv_panic_safe_putchar(str[idx]);
    idx++;
  }
}

bool memfault_port_coredump_save_begin(void) {
  // Update Memfault task watchdog bookkeeping, if it's enabled
  memfault_task_watchdog_bookkeep();

  // Disable the interrupt watchdog (IWDT) if it's enabled, to avoid contention
  // with the IWDT interrupt when the fault handler is executing. This is safe
  // to do when we're in the panic handler, because ESP-IDF's
  // esp_panic_handler() has already enabled the WDT_RWDT to hard-reset the chip
  // if the panic handler hangs.
#if defined(CONFIG_ESP_INT_WDT)
  // Define ETS_INT_WDT_INUM for compatibility with < 5.1
  #if !defined(ETS_INT_WDT_INUM)
    #define ETS_INT_WDT_INUM (ETS_T1_WDT_INUM)
  #endif
  esp_intr_disable_source(ETS_INT_WDT_INUM);
#endif  // defined(CONFIG_ESP_INT_WDT)

  prv_panic_safe_putstr("Saving Memfault Coredump!\r\n");
  return (memfault_esp_spi_flash_coredump_begin() == 0);
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data, size_t data_len) {
  const esp_partition_t *core_part = prv_get_core_partition();
  if (core_part == NULL) {
    return false;
  }

  const size_t address =
    core_part->address + offset +
    (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE);
  const esp_err_t err = memfault_esp_spi_flash_write(address, data, data_len);
  if (err != ESP_OK) {
    prv_panic_safe_putstr("coredump write failed");
  }
  return (err == ESP_OK);
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  const esp_partition_t *core_part = prv_get_core_partition();
  if (core_part == NULL) {
    return false;
  }
  if ((offset + read_len) > core_part->size) {
    return false;
  }
  const uint32_t address =
    core_part->address + offset +
    (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE);
  const esp_err_t err = memfault_esp_spi_flash_read(address, data, read_len);
  return (err == ESP_OK);
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  const esp_partition_t *core_part = prv_get_core_partition();
  if (core_part == NULL) {
    return false;
  }

  const size_t address = core_part->address + offset;
  // include the write offset to set the erase size, but erase starting from the
  // base address so the full partition will be erased
  erase_size += (CONFIG_MEMFAULT_COREDUMP_STORAGE_WRITE_OFFSET_SECTORS * SPI_FLASH_SEC_SIZE);
  const esp_err_t err = memfault_esp_spi_flash_erase_range(address, erase_size);
  if (err != ESP_OK) {
    prv_panic_safe_putstr("coredump erase failed");
  }

  return (err == ESP_OK);
}
