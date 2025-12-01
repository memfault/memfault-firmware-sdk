//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! RRAM backed coredump storage implementation. To make use of this, be sure to
//! add a fixed partition named "memfault_coredump_partition" to your device
//! tree or use partition manager to define one.
//!
//! For example, if using partition manager, you might add an entry like this to
//! the application's pm_static.yml file:
//!
//!  memfault_coredump_partition:
//!    address: 0x1fc000
//!    end_address: 0x1fd000
//!    placement:
//!      after:
//!      - mcuboot_secondary
//!    region: flash_primary
//!    size: 0x1000
//!
//! If using device tree specified partitions, you might add something like this
//! to your board's dts file/overlay:
//!
//!  &mram1x {
//!    partitions {
//!      memfault_coredump_partition: partition@1d5000 {
//!        label = "memfault_coredump_partition";
//!        reg = <0x1d5000 0x5000>;
//!      };
//!    };
//!  };

#include <hal/nrf_rramc.h>
#include <memfault/components.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/barrier.h>

//! nRF54 series RRAM uses a 128-bit wordline- writing smaller amounts still
//! causes a full wordline write, so to make the best use of write cycles, we
//! buffer writes to this size.
#define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (128 / 8)
#include <memfault/ports/buffered_coredump_storage.h>

// Ensure the memfault_coredump_partition entry exists

// Fixed partitions in use
#if DT_HAS_FIXED_PARTITION_LABEL(memfault_coredump_partition)
  #define MEMFAULT_COREDUMP_PARTITION_OFFSET \
    DT_FIXED_PARTITION_ADDR(DT_NODELABEL(memfault_coredump_partition))
  #define MEMFAULT_COREDUMP_PARTITION_SIZE DT_REG_SIZE(DT_NODELABEL(memfault_coredump_partition))

// Partition-manager defined partitions
#elif FIXED_PARTITION_EXISTS(memfault_coredump_partition)
  #define MEMFAULT_COREDUMP_PARTITION_OFFSET FIXED_PARTITION_OFFSET(memfault_coredump_partition)
  #define MEMFAULT_COREDUMP_PARTITION_SIZE FIXED_PARTITION_SIZE(memfault_coredump_partition)
#else
  #error "Be sure to add a fixed partition named 'memfault_coredump_partition'!"
#endif

MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_PARTITION_SIZE % MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE == 0,
                       "Storage size must be a multiple of 128-bit (16 bytes)");

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
  const uint32_t first_wordline = *(const uint32_t *)(MEMFAULT_COREDUMP_PARTITION_OFFSET);
  if (first_wordline == 0) {
    memset(data, 0, read_len);
    return true;
  }

  // RRAM is memory mapped, so we can just read it directly
  const uint32_t address = MEMFAULT_COREDUMP_PARTITION_OFFSET + offset;

  memcpy(data, (void *)address, read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  // Erase is only ever called just prior to writing a new coredump or in debug
  // testing, so we only need to wipe the first 32 bits. We'll overwrite the
  // first wordline for simplicity.
  uint8_t erase_buf[MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE] = { 0 };

  return memfault_platform_coredump_storage_write(offset, erase_buf, sizeof(erase_buf));
}

bool memfault_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  const uint32_t start_addr = MEMFAULT_COREDUMP_PARTITION_OFFSET;
  const uint32_t addr = start_addr + blk->write_offset;

  if (!prv_op_within_flash_bounds(blk->write_offset, MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE)) {
    return false;
  }

  // save existing rramc config
  nrf_rramc_config_t prior_config;
  nrf_rramc_config_get(NRF_RRAMC, &prior_config);

  // enable writing
  nrf_rramc_config_t config = {
    .mode_write = true,
    .write_buff_size = MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE,
  };

  nrf_rramc_config_set(NRF_RRAMC, &config);

  // write the data
  memcpy((void *)addr, blk->data, MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE);

  // trigger the commit
  nrf_rramc_task_trigger(NRF_RRAMC, NRF_RRAMC_TASK_COMMIT_WRITEBUF);

  // force a bus synchronization to avoid any issues with cached reads
  barrier_dmem_fence_full();

  // disable writing
  config.mode_write = false;
  nrf_rramc_config_set(NRF_RRAMC, &config);

  // restore previous config
  nrf_rramc_config_set(NRF_RRAMC, &prior_config);

  return true;
}

//! Primarily used for debug. Call erase to wipe out the coredump magic.
void memfault_platform_coredump_storage_clear(void) {
  memfault_platform_coredump_storage_erase(0, 0);
}
