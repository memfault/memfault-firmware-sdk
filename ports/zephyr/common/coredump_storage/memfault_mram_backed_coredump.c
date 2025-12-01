//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! MRAM backed coredump storage implementation. To make use of this, be sure to
//! add a fixed partition named "memfault_coredump_partition" to your device
//! tree or use partition manager to define one.
//!
//! For example, if using partition manager, you might add an entry like this to
//! the application's pm_static.yml file:
//!
//!  memfault_coredump_partition:
//!    address: 0x155000
//!    end_address: 0x165000
//!    placement:
//!      align:
//!        start: 0x1000
//!      before:
//!      - end
//!    region: flash_primary
//!    size: 0x10000
//!
//! If using device tree specified partitions, you might add something like this
//! to your board's dts file/overlay:
//!
//!  &mram1x {
//!    partitions {
//!      memfault_coredump_partition: partition@1a9000 {
//!        reg = <0x1a9000 DT_SIZE_K(20)>;
//!      };
//!    };
//!  };

#include <memfault/components.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

//! nRF54H20 MRAM has a 16-byte write block size
#define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (16)
#include <memfault/ports/buffered_coredump_storage.h>

// Ensure the memfault_coredump_partition entry exists
#if !FIXED_PARTITION_EXISTS(memfault_coredump_partition)
  #error "Be sure to add a fixed partition named 'memfault_coredump_partition'!"
#endif

#if defined(CONFIG_SOC_NRF54H20)
  #define MEMFAULT_COREDUMP_FLASH_DEVICE DEVICE_DT_GET(DT_CHOSEN(zephyr_flash))
  #define MEMFAULT_COREDUMP_PARTITION_OFFSET FIXED_PARTITION_OFFSET(memfault_coredump_partition)
  #define MEMFAULT_COREDUMP_PARTITION_SIZE FIXED_PARTITION_SIZE(memfault_coredump_partition)
#else
  #error "Unknown SOC, please contact support: https://mflt.io/contact-support"
#endif

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
  uint32_t first_word = 0xffffffff;
  (int)flash_read(MEMFAULT_COREDUMP_FLASH_DEVICE, MEMFAULT_COREDUMP_PARTITION_OFFSET, &first_word,
                  sizeof(first_word));
  if (first_word == 0) {
    memset(data, 0, read_len);
    return true;
  }

  // read the data
  (int)flash_read(MEMFAULT_COREDUMP_FLASH_DEVICE, MEMFAULT_COREDUMP_PARTITION_OFFSET + offset, data,
                  read_len);
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

  (int)flash_write(MEMFAULT_COREDUMP_FLASH_DEVICE, addr, blk->data, sizeof(blk->data));

  return true;
}

//! Primarily used for debug. Call erase to wipe out the coredump magic.
void memfault_platform_coredump_storage_clear(void) {
  memfault_platform_coredump_storage_erase(0, 0);
}
