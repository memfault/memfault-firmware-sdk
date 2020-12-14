//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief

#include "memfault/panics/platform/coredump.h"

#include <zephyr.h>
#include <kernel.h>
#include <kernel_structs.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/nrfconnect_port/coredump.h"

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_coredump") MEMFAULT_ALIGNED(8)
static uint8_t s_ram_backed_coredump_region[CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE];

#define EXTRA_REGIONS (   \
  1 /* active stack */ + \
  1 /* _kernel variable */ + \
  1 /* data region */ + \
  1 /* bss region */ )

static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_COREDUMP_MAX_TASK_REGIONS + EXTRA_REGIONS];

MEMFAULT_WEAK
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {

   int region_idx = 0;
   s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
       crash_info->stack_address, CONFIG_MEMFAULT_COREDUMP_STACK_SIZE_TO_COLLECT);
   *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
   region_idx++;

   s_coredump_regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
       &_kernel, sizeof(_kernel));
   *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
   region_idx++;

   region_idx += memfault_nrfconnect_get_task_regions(
       &s_coredump_regions[region_idx],
       MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

   //
   // Now that we have captures all the task state, we will
   // fill whatever space remains in coredump storage with the
   // data and bss we can collect!
   //

   region_idx +=  memfault_nrfconnect_get_data_regions(
       &s_coredump_regions[region_idx],
       MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

   region_idx +=  memfault_nrfconnect_get_bss_regions(
       &s_coredump_regions[region_idx],
       MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

   *num_regions = region_idx;
   return &s_coredump_regions[0];
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo) {
    .size = sizeof(s_ram_backed_coredump_region),
    .sector_size = sizeof(s_ram_backed_coredump_region),
  };
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((offset + read_len) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  const uint8_t *read_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(data, read_ptr, read_len);
  return true;
}


bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  uint8_t *erase_ptr = &s_ram_backed_coredump_region[offset];
  memset(erase_ptr, 0x0, erase_size);
  return true;
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  uint8_t *write_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

void memfault_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  memfault_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}
