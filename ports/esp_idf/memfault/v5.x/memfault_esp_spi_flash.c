//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "esp_flash.h"
#include "esp_flash_internal.h"
#include "esp_private/spi_flash_os.h"
#include "memfault/core/math.h"
#include "memfault/esp_port/spi_flash.h"
#include "spi_flash_mmap.h"

int memfault_esp_spi_flash_coredump_begin(void) {
  // re-configure flash driver to be call-able from an Interrupt context

  spi_flash_guard_set(&g_flash_guard_no_os_ops);
  return esp_flash_app_disable_protect(true);
}

int memfault_esp_spi_flash_erase_range(size_t start_address, size_t size) {
  return esp_flash_erase_region(esp_flash_default_chip, start_address, size);
}

int memfault_esp_spi_flash_write(size_t dest_addr, const void *src, size_t size) {
#if defined(CONFIG_SPI_FLASH_VERIFY_WRITE)
  uint8_t temp_buf[128];
  uintptr_t src_end = (uintptr_t)src + size;

  // Use a temp buffer to avoid issues where the source data changes
  // during the write verification
  for (const uint8_t *src_pointer = src; (uintptr_t)src_pointer < src_end;
       src_pointer += sizeof(temp_buf)) {
    // Chunk the source buffer into temp_buf segments or remaining data, whichever is smaller
    size_t chunk_size = MEMFAULT_MIN(sizeof(temp_buf), (src_end - (uintptr_t)src_pointer));
    memcpy(temp_buf, src_pointer, chunk_size);

    // Write the temp buffer to flash, bail early if we hit an error
    int err =
      esp_flash_write(esp_flash_default_chip, temp_buf,
                      (uintptr_t)dest_addr + ((uintptr_t)src_pointer - (uintptr_t)src), chunk_size);
    if (err != 0) {
      return err;
    }
  }

  return 0;
#else
  return esp_flash_write(esp_flash_default_chip, src, dest_addr, size);
#endif  // defined(CONFIG_SPI_FLASH_VERIFY_WRITE)
}

int memfault_esp_spi_flash_read(size_t src_addr, void *dest, size_t size) {
  return esp_flash_read(esp_flash_default_chip, dest, src_addr, size);
}
