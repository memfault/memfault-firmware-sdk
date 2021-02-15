//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A port for the platform dependencies needed to use the coredump feature from the "panics"
//! component.
//!
//! This can be linked in directly by adding the .c file to the build system or can be
//! copied into your repo and modified to collect different RAM regions.
//!
//! By default, it will collect the top 300 bytes of the stack which was running at the time of the
//! crash. This allows for a reasonable backtrace to be collected while using very little RAM.
//!
//! The only change needed to integrate the port is to place the "s_ram_backed_coredump_region"
//! static buffer into a NOINIT section. For GNU GCC, this means you will want to add something
//! like the following to your linker script:
//!
//! /* Your .ld file */
//! MEMORY
//! {
//!   [...]
//!   COREDUMP_NOINIT (rw) :  ORIGIN = <RAM_REGION_START>, LENGTH = 700
//! }
//! SECTIONS
//! {
//!  [...]
//!  .coredump_noinit (NOLOAD): { KEEP(*(*.mflt_coredump)) } > COREDUMP_NOINIT
//! }

#include "memfault/panics/platform/coredump.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "memfault/config.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"

#ifndef MEMFAULT_RAM_BACK_COREDUMP_STACK_COLLECTION_SIZE
#define MEMFAULT_RAM_BACK_COREDUMP_STACK_COLLECTION_SIZE 200
#endif /* MEMFAULT_RAM_BACK_COREDUMP_STACK_COLLECTION_SIZE */

#ifndef MEMFAULT_RAM_BACKED_COREDUMP_SIZE
#define MEMFAULT_RAM_BACKED_COREDUMP_SIZE \
  (500 + MEMFAULT_RAM_BACK_COREDUMP_STACK_COLLECTION_SIZE)
#endif /* MEMFAULT_RAM_BACKED_COREDUMP_SIZE */

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_coredump") MEMFAULT_ALIGNED(8)
static uint8_t s_ram_backed_coredump_region[MEMFAULT_RAM_BACKED_COREDUMP_SIZE];

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
   static sMfltCoredumpRegion s_coredump_regions[1];

   s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
       crash_info->stack_address, MEMFAULT_RAM_BACK_COREDUMP_STACK_COLLECTION_SIZE);
   *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
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
