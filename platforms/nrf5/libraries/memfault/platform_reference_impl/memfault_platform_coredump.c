//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions which could be used
//! for coredump collection on the NRF52

#include "memfault/panics/coredump.h"

#include <string.h>

#include "app_util.h"
#include "nrf_log.h"
#include "nrf_nvmc.h"
#include "nrf_sdh.h"

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/panics/platform/coredump.h"

#ifndef MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY
#  define MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY 1
#endif

// Linker defined symbols specifying the region to store coredumps to
//
// NOTE: The size of the regions you collect must be less than the size
// of the space reserved for saving coredumps!
extern uint32_t __CoreStart;
extern uint32_t __CoreEnd;

//! To capture the entire "RAM" region, define these symbols in the .ld script
//! of your project:
//!
//! .mflt_coredump_symbols :
//! {
//!   __MfltCoredumpRamStart = ORIGIN(RAM);
//!   __MfltCoredumpRamEnd = ORIGIN(RAM) + LENGTH(RAM);
//! } > FLASH
extern uint32_t __MfltCoredumpRamStart;
extern uint32_t __MfltCoredumpRamEnd;

#define CORE_REGION_START ((uint32_t)&__CoreStart)
#define CORE_REGION_LENGTH ((uint32_t)&__CoreEnd - CORE_REGION_START)

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                                                  size_t *num_regions) {
  // Let's collect the callstack at the time of crash
  static sMfltCoredumpRegion s_coredump_regions[1];

#if (MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY == 1)
  // Capture only the interrupt stack. Use only if there is not enough storage to capture all of RAM.
  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(STACK_BASE, (uint32_t)STACK_TOP - (uint32_t)STACK_BASE);
#else
  // Capture all of RAM. Recommended: it enables broader post-mortem analyses, but has larger storage requirements.
  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&__MfltCoredumpRamStart,
      (uint32_t)&__MfltCoredumpRamEnd - (uint32_t)&__MfltCoredumpRamStart);
#endif

  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);

  return s_coredump_regions;
}

void memfault_platform_coredump_storage_clear(void) {
  // static because sd_flash_write may take place asynchrnously
  const static uint32_t invalidate = 0x0;

  // If the soft device is enabled we need to route our flash operation through it (Code saving
  // backtraces hits the flash directly since the soft device is out of the picture at the time we
  // crash!)

  if (nrf_sdh_is_enabled()) {
    uint32_t rv = sd_flash_write((void *)CORE_REGION_START, &invalidate, 1);
    if (rv != NRF_SUCCESS) {
      NRF_LOG_ERROR("Unable to mark coredump as read, %d", rv);
    }
    // TODO: Should really wait for NRF_EVT_FLASH_OPERATION_SUCCESS or retry
    // on error from the soft device
  } else {
    nrf_nvmc_write_word(CORE_REGION_START, invalidate);
  }
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info  = (sMfltCoredumpStorageInfo) {
    .size = CORE_REGION_LENGTH,
    .sector_size = NRF_FICR->CODEPAGESIZE,
  };
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > CORE_REGION_LENGTH) {
    return false;
  }

  uint32_t address = CORE_REGION_START + offset;

  const uint32_t word_size = 4;
  if (((address % word_size) == 0) && (((uint32_t)data % word_size) == 0)) {
    const size_t num_words = data_len / word_size;
    nrf_nvmc_write_words(address, data, num_words);
    data_len = data_len % word_size;
    const size_t bytes_written = num_words * word_size;
    address += num_words * word_size;
    const uint8_t *data_rem = data;
    data_rem += bytes_written;
    nrf_nvmc_write_bytes(address, data_rem, data_len);
  } else {
    nrf_nvmc_write_bytes(address, data, data_len);
  }
  return true;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((offset + read_len) > CORE_REGION_LENGTH) {
    return false;
  }

  uint32_t address = CORE_REGION_START + offset;

  memcpy(data, (void *)address, read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > CORE_REGION_LENGTH) {
    return false;
  }

  const size_t sector_size = NRF_FICR->CODEPAGESIZE;

  if ((offset % sector_size) != 0) {
    return false;
  }

  for (size_t sector = offset; sector < erase_size; sector += sector_size) {
    uint32_t  address = CORE_REGION_START + sector;
    nrf_nvmc_page_erase(address);
  }

  return true;
}
