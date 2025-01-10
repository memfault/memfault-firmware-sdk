//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Reference implementation of platform dependency functions to use a sector of
//! internal flash for coredump capture
//!
//! To use, update your linker script (.ld file) to expose information about the
//! location to use. For example, using the last 4 sectors of the STM32U535/545
//! (512K flash) would look like this:
//!
//! MEMORY
//! {
//!     /* ... other regions ... */
//!     COREDUMP_STORAGE_FLASH (r) : ORIGIN = 0x0807C000, LENGTH = 4 * 8K
//! }
//! __MemfaultCoreStorageStart = ORIGIN(COREDUMP_STORAGE_FLASH);
//! __MemfaultCoreStorageEnd = ORIGIN(COREDUMP_STORAGE_FLASH) + LENGTH(COREDUMP_STORAGE_FLASH);

#include <string.h>

#include "memfault/config.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"
#include "stm32u5xx_hal.h"

#if defined(FLASH_TYPEPROGRAM_DOUBLEWORD)
  #define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (2 * 4)
#elif defined(FLASH_TYPEPROGRAM_QUADWORD)
  #define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (4 * 4)
#else
  #error "Unsupported FLASH_TYPEPROGRAM"
#endif
#include "memfault/ports/buffered_coredump_storage.h"

#ifndef MEMFAULT_COREDUMP_STORAGE_START_ADDR
extern uint32_t __MemfaultCoreStorageStart[];
  #define MEMFAULT_COREDUMP_STORAGE_START_ADDR ((uint32_t)__MemfaultCoreStorageStart)
#endif

#ifndef MEMFAULT_COREDUMP_STORAGE_END_ADDR
extern uint32_t __MemfaultCoreStorageEnd[];
  #define MEMFAULT_COREDUMP_STORAGE_END_ADDR ((uint32_t)__MemfaultCoreStorageEnd)
#endif

#ifndef MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE
//! STM32U5 series chips have 8kB page size
  #define MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE (8 * 1024)
#endif

#define FLASH_BANK_1_START_ADDR 0x08000000
#if defined(STM32U585xx)
  #define FLASH_BANK_2_START_ADDR 0x08100000
#elif defined(STM32U5G9xx)
  #define FLASH_BANK_2_START_ADDR 0x08200000
#else
  #error Unsupported STM32U5 series chip, please contact https://mflt.io/contact-support
#endif

#ifndef MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_NUMBER
//! The sector number to write to. Computed based on the start address, and
//! offset by the bank start offset for the chip
  #define MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_NUMBER                     \
    (((MEMFAULT_COREDUMP_STORAGE_START_ADDR >= FLASH_BANK_2_START_ADDR) ?   \
        (MEMFAULT_COREDUMP_STORAGE_START_ADDR - FLASH_BANK_2_START_ADDR) :  \
        (MEMFAULT_COREDUMP_STORAGE_START_ADDR - FLASH_BANK_1_START_ADDR)) / \
     MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE)
#endif

// Error writing to flash - should never happen & likely detects a configuration error
// Call the reboot handler which will halt the device if a debugger is attached and then reboot
MEMFAULT_NO_OPT static void prv_coredump_writer_assert_and_reboot(int error_code) {
  (void)error_code;
  memfault_platform_halt_if_debugging();
  memfault_platform_reboot();
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sMfltCoredumpStorageInfo info = { 0 };
  memfault_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

void memfault_platform_coredump_storage_clear(void) {
  memfault_platform_coredump_storage_erase(0, MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE);
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  const size_t size = MEMFAULT_COREDUMP_STORAGE_END_ADDR - MEMFAULT_COREDUMP_STORAGE_START_ADDR;

  *info = (sMfltCoredumpStorageInfo){
    .size = size,
    .sector_size = MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE,
  };
}

bool memfault_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  const uint32_t write_offset = blk->write_offset;
  if (!prv_op_within_flash_bounds(write_offset, sizeof(blk->data))) {
    return false;
  }

  HAL_FLASH_Unlock();
  MEMFAULT_STATIC_ASSERT(sizeof(blk->data) == MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE,
                         "Invalid write size");
  const uint32_t res = HAL_FLASH_Program(
#if defined(FLASH_TYPEPROGRAM_DOUBLEWORD)
    FLASH_TYPEPROGRAM_DOUBLEWORD,
#elif defined(FLASH_TYPEPROGRAM_QUADWORD)
    FLASH_TYPEPROGRAM_QUADWORD,
#endif
    write_offset + MEMFAULT_COREDUMP_STORAGE_START_ADDR, (uint32_t)blk->data);
  if (res != HAL_OK) {
    prv_coredump_writer_assert_and_reboot(res);
  }
  HAL_FLASH_Lock();
  return true;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  // The internal flash is memory mapped so we can just use memcpy!
  const uint32_t start_addr = MEMFAULT_COREDUMP_STORAGE_START_ADDR;
  memcpy(data, (void *)(start_addr + offset), read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  // Compute the flash bank the coredump storage is in
  const uint32_t bank = (MEMFAULT_COREDUMP_STORAGE_START_ADDR >= FLASH_BANK_2_START_ADDR) ?
                          (FLASH_BANK_2) :
                          (FLASH_BANK_1);

  FLASH_EraseInitTypeDef s_erase_cfg = {
    .TypeErase = FLASH_TYPEERASE_PAGES,
    .Banks = bank,
    .Page = MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_NUMBER +
            (offset / MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE),
    .NbPages = erase_size / MEMFAULT_COREDUMP_STORAGE_FLASH_SECTOR_SIZE,
  };
  uint32_t SectorError = 0;
  HAL_FLASH_Unlock();
  {
    int res = HAL_FLASHEx_Erase(&s_erase_cfg, &SectorError);
    if (res != HAL_OK) {
      prv_coredump_writer_assert_and_reboot(res);
    }
  }
  HAL_FLASH_Lock();

  return true;
}
