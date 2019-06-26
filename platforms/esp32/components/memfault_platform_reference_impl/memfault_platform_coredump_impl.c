//! @file
//!
//! Reference implementation of platform dependency functions which could be used
//! for coredump collection on the ESP32

#include "memfault/panics/coredump.h"

#include <string.h>

#include "esp_partition.h"
#include "esp_spi_flash.h"

#include "memfault/core/compiler.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/core/platform/debug_log.h"

// Copied from esp-idf/components/esp32/core_dump.c
#define ESP_IDF_COREDUMP_FLASH_MAGIC_START    0xE32C04EDUL

// Copied from esp-idf/components/esp32/core_dump.c
typedef struct _core_dump_header_t
{
  uint32_t data_len;  // data length
  uint32_t tasks_num; // number of tasks
  uint32_t tcb_sz;    // size of TCB
} core_dump_header_t;

bool memfault_coredump_get_esp_idf_formatted_coredump_size(size_t *size_out) {
  struct MEMFAULT_PACKED {
    uint32_t magic;
    core_dump_header_t hdr;
  } header;
  if (!memfault_platform_coredump_storage_read(0, &header, sizeof(header))) {
    return false;
  }
  if (header.magic != ESP_IDF_COREDUMP_FLASH_MAGIC_START) {
    return false;
  }
  sMfltCoredumpStorageInfo storage_info;
  memfault_platform_coredump_storage_get_info(&storage_info);
  if (storage_info.size < header.hdr.data_len) {
    return false;
  }
  *size_out = header.hdr.data_len;
  return true;
}

static const esp_partition_t *prv_get_core_part(void) {
  // The esp_partition object is created once upon first request and never free'd -- see partition.c:
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(size_t *num_regions) {
  // TODO: Not implemented yet; relying on esp-idf's esp_core_dump_to_flash() function
  // to capture coredumps as a first step.
  return NULL;
}

void memfault_platform_coredump_storage_clear(void) {
  const esp_partition_t *core_part = prv_get_core_part();
  if (!core_part) {
    return;
  }
  const uint32_t invalidate = 0x0;
  if (core_part->size <= sizeof(invalidate)) {
    return;
  }
  const esp_err_t err = spi_flash_write(core_part->address, &invalidate, sizeof(invalidate));
  if (err != ESP_OK) {
    memfault_platform_log(kMemfaultPlatformLogLevel_Error, "Failed to write data to flash (%d)!", err);
  }
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  const esp_partition_t *core_part = prv_get_core_part();
  if (!core_part) {
    *info = (sMfltCoredumpStorageInfo) { 0 };
    return;
  }
  *info  = (sMfltCoredumpStorageInfo) {
    .size = core_part->size,
    .sector_size = SPI_FLASH_SEC_SIZE,
  };
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  // TODO: Not implemented yet; relying on esp-idf's esp_core_dump_to_flash() function
  // to capture coredumps as a first step.
  return false;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  const esp_partition_t *core_part = prv_get_core_part();
  if (!core_part) {
    return false;
  }
  if ((offset + read_len) > core_part->size) {
    return false;
  }
  const uint32_t address = core_part->address + offset;
  const esp_err_t err = spi_flash_read(address, data, read_len);
  if (err != ESP_OK) {
    memfault_platform_log(kMemfaultPlatformLogLevel_Error, "Failed to read data from flash (%d)!", err);
  }
  return (err == ESP_OK);
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  // TODO: Not implemented yet; relying on esp-idf's esp_core_dump_to_flash() function
  // to capture coredumps as a first step.
  return false;
}
