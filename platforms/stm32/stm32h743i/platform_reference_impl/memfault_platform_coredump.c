//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions which could be used
//! for coredump collection on an STM32H7

#include "memfault/panics/coredump.h"

#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/panics/platform/coredump.h"

#include "stm32h7xx_hal_flash.h"

// NOTE: Region to be used for saving coredump

// NOTE: The stm32h743zi has two flash "banks" each of size 1MB ranging
// from 0x8000000 - 0x8200000
//
// As an example, we will store crash information in the last sector of the second bank
struct { //
  uint32_t bank_start_addr;
  // the offset within the bank to start writing coredumps at
  uint32_t bank_start_off;
  // the offset within the bank to stop writing coredumps at
  uint32_t bank_end_off;
  // the id of the bank
  uint32_t bank_id;
  // the size of an erase sector
  uint32_t sector_size;
} s_coredump_flash_storage = {
  .bank_start_addr = 0x8100000,
  .bank_start_off = 0x0E0000,
  .bank_end_off = 0x100000,
  .bank_id = FLASH_BANK_2,
  .sector_size = 128 * 1024,
};

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                                                  size_t *num_regions) {
  static sMfltCoredumpRegion s_coredump_regions[2];
  // NOTE: This is just an example of regions which could be collected
  // Typically sizes are derived from variables added to the .ld script of the port

  // Beginning of DTCM-RAM (Total size is 128kB)
  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT((void *)0x20000000,
      32*1024);
  // Beginning of AXI SRAM (Total size is 512kB)
  s_coredump_regions[1] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT((void *)0x24000000,
      32*1024);

  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
  return s_coredump_regions;
}

// Error writing to flash - should never happen & likely detects a configuration error
// Call our reboot handler which will halt the device if a debugger is attached and then reboot
__attribute__((optimize("O0")))
__attribute__((noinline))
static void prv_coredump_writer_assert_and_reboot(int error_code) {
  memfault_platform_reboot();
}

void memfault_platform_coredump_storage_clear(void) {
  const uint32_t sector_id = s_coredump_flash_storage.bank_start_off / s_coredump_flash_storage.sector_size;
  FLASH_EraseInitTypeDef s_erase_cfg = {
    .TypeErase = FLASH_TYPEERASE_SECTORS,
    .Banks = s_coredump_flash_storage.bank_id,
    .Sector = sector_id,
    .NbSectors = 1,
    .VoltageRange = FLASH_VOLTAGE_RANGE_4
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
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info  = (sMfltCoredumpStorageInfo) {
    .size = s_coredump_flash_storage.bank_end_off - s_coredump_flash_storage.bank_start_off,
    .sector_size = s_coredump_flash_storage.sector_size,
  };
}

// NOTE: The internal STM32H74 flash uses 10 ECC bits over 32 byte "memory words". Since the ECC
// bits are also in NOR flash, this means 32 byte hunks can only be written once since an updated
// in the future would likely cause the ECC to go bad.
//
// In practice this means, writes must be issued 32 bytes at a time. The code below accomplishes this
// by only issuing writes when 32 bytes have been collected. The Memfault coredump writer is guaranteed
// to issue writes sequentially with the exception of the header which is at the beginning of the coredump
// region and written last. A future update of the Memfault SDK will incorporate this logic inside the SDK
// itself.

#define COREDUMP_STORAGE_WRITE_SIZE 32
typedef struct {
  uint8_t data[COREDUMP_STORAGE_WRITE_SIZE];
  uint32_t address;
  uint32_t bytes_written;
} sCoredumpWorkingBuffer;

MEMFAULT_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer_header;
MEMFAULT_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer;

static sCoredumpWorkingBuffer *prv_get_working_buf(uint32_t offset) {
  return (offset == 0) ? &s_working_buffer_header : &s_working_buffer;
}

static void prv_write_block(sCoredumpWorkingBuffer *blk) {
  const uint32_t start_addr = s_coredump_flash_storage.bank_start_addr + s_coredump_flash_storage.bank_start_off;
  const uint32_t addr = start_addr + blk->address;

  HAL_FLASH_Unlock();
  {
    const uint32_t res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr, (uint32_t)blk->data);
    if (res != HAL_OK) {
      prv_coredump_writer_assert_and_reboot(res);
    }
  }
  HAL_FLASH_Lock();

  *blk = (sCoredumpWorkingBuffer){ 0 };
}

static void prv_try_flush(void) {
  sCoredumpWorkingBuffer *hdr_block = &s_working_buffer_header;
  sCoredumpWorkingBuffer *data_block = &s_working_buffer;

  if (hdr_block->bytes_written == COREDUMP_STORAGE_WRITE_SIZE) {
    prv_write_block(hdr_block);

    // the header is flushed last so if we still have data we need to flush that too
    if (data_block->bytes_written != 0) {
      prv_write_block(data_block);
    }
  }

  if (data_block->bytes_written == COREDUMP_STORAGE_WRITE_SIZE) {
    prv_write_block(data_block);
  }
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((s_coredump_flash_storage.bank_start_off + offset + data_len) >
      s_coredump_flash_storage.bank_end_off) {
    return false;
  }

  const uint8_t *datap = data;
  uint32_t start_addr = offset;
  uint32_t page_aligned_start_address =
      (start_addr / COREDUMP_STORAGE_WRITE_SIZE) * COREDUMP_STORAGE_WRITE_SIZE;

  // we have to copy data into a temporary buffer because we can only issue 32
  // byte aligned writes that are 32 bytes in length
  if (page_aligned_start_address != start_addr) {
    sCoredumpWorkingBuffer *working_buffer =
        prv_get_working_buf(page_aligned_start_address);
    uint32_t bytes_to_write = MEMFAULT_MIN(
        (page_aligned_start_address + COREDUMP_STORAGE_WRITE_SIZE) - start_addr,
        data_len);
    uint32_t write_offset = start_addr - page_aligned_start_address;
    memcpy(&working_buffer->data[write_offset], datap, bytes_to_write);
    working_buffer->bytes_written += bytes_to_write;
    working_buffer->address = page_aligned_start_address;
    prv_try_flush();

    start_addr += bytes_to_write;
    data_len -= bytes_to_write;
    datap += bytes_to_write;
  }

  for (uint32_t i = 0; i < data_len; i += COREDUMP_STORAGE_WRITE_SIZE) {
    const uint32_t size =
        MEMFAULT_MIN(COREDUMP_STORAGE_WRITE_SIZE, data_len - i);
    sCoredumpWorkingBuffer *working_buffer =
        prv_get_working_buf(start_addr + i);
    memcpy(&working_buffer->data, &datap[i], size);
    working_buffer->bytes_written += size;
    working_buffer->address = start_addr + i;
    prv_try_flush();
  }

  return true;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((s_coredump_flash_storage.bank_start_off + offset + read_len) > s_coredump_flash_storage.bank_end_off) {
    return false;
  }

  // The internal flash is memory mapped
  const uint32_t start_addr = s_coredump_flash_storage.bank_start_addr + s_coredump_flash_storage.bank_start_off;
  memcpy(data, (void *)(start_addr + offset), read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  // NOTE: For now, we will just clear the entire region for coredumps when an erase is issued
  // but this could just erase the sectors covered by erase_size
  memfault_platform_coredump_storage_clear();
  return true;
}
