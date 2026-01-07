//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! MRAM backed coredump storage implementation. To make use of this, be sure to
//! add a fixed partition named "memfault_coredump_partition" to your device
//! tree :
//!
//! &flash0 {
//!   partitions {
//!     memfault_coredump_partition: partition@1e6000 {
//!       reg = <0x1e6000 0x2000>;
//!     };
//!   };
//! };

#include <memfault/components.h>
#include <soc.h>

// Note: this is only used for the FIXED_PARTITION_OFFSET/ SIZE macros. The
// FLASH_AMBIQ driver uses a semaphore for concurrent access protection, which
// prohibits using it from the fault handler context.
#include <zephyr/storage/flash_map.h>

//! Apollo4 MRAM has a 16-byte write block size
#define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (16)
#include <memfault/ports/buffered_coredump_storage.h>

// Ensure the memfault_coredump_partition entry exists
#if !FIXED_PARTITION_EXISTS(memfault_coredump_partition)
  #error "Be sure to add a fixed partition named 'memfault_coredump_partition'!"
#endif

#define MEMFAULT_COREDUMP_PARTITION_OFFSET FIXED_PARTITION_OFFSET(memfault_coredump_partition)
#define MEMFAULT_COREDUMP_PARTITION_SIZE FIXED_PARTITION_SIZE(memfault_coredump_partition)

MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_PARTITION_SIZE % MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE == 0,
                       "Storage size must be a multiple of (16 bytes)");

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo){
    .size = MEMFAULT_COREDUMP_PARTITION_SIZE,
  };
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sMfltCoredumpStorageInfo info = { 0 };

  memfault_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  // special case: if the first word is 0, the coredump is cleared, and reads
  // should return all zeros
  uint32_t first_word = *(volatile uint32_t *)(MEMFAULT_COREDUMP_PARTITION_OFFSET);
  if (first_word == 0) {
    memset(data, 0, read_len);
    return true;
  }

  // read the data directly from flash memory
  memcpy(data, (uint8_t *)(MEMFAULT_COREDUMP_PARTITION_OFFSET + offset), read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  // Erase is only ever called just prior to writing a new coredump or in debug
  // testing, so we only need to wipe the first 32 bits. We'll overwrite the
  // first unit of write size for simplicity.
  uint8_t erase_buf[sizeof(((sCoredumpWorkingBuffer *)0)->data)] = { 0 };

  return memfault_platform_coredump_storage_write(offset, erase_buf, sizeof(erase_buf));
}

bool memfault_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  const uint32_t addr = MEMFAULT_COREDUMP_PARTITION_OFFSET + blk->write_offset;

  if (!prv_op_within_flash_bounds(blk->write_offset, sizeof(blk->data))) {
    return false;
  }

  // Prepare aligned buffer for MRAM write (requires 4-byte aligned data)
  uint32_t aligned[sizeof(blk->data) / sizeof(uint32_t)];
  uint32_t *src = (uint32_t *)blk->data;

  for (int i = 0; i < sizeof(blk->data) / sizeof(uint32_t); i++) {
    aligned[i] = UNALIGNED_GET((uint32_t *)src);
    src++;
  }

  int ret = am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY, aligned, (uint32_t *)addr,
                                     sizeof(blk->data) / sizeof(uint32_t));

  return (ret == 0);
}

//! Primarily used for debug. Call erase to wipe out the coredump magic.
void memfault_platform_coredump_storage_clear(void) {
  memfault_platform_coredump_storage_erase(0, 0);
}
