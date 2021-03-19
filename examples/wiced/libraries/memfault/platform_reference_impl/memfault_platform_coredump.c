//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions which could be used
//! for coredump collection on the WICED platform

#include "memfault/panics/coredump.h"

#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/core/platform/crc32.h"

// The ordering here is deliberate because some includes are missing in these headers :/
#include "wiced_result.h"
#include "platform_dct.h"
#include "platform_peripheral.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "spi_flash.h"
#include "wiced_apps_common.h"
#include "wiced_utilities.h"
#include "wiced_waf_common.h"

#ifndef PLATFORM_SFLASH_PERIPHERAL_ID
#  define PLATFORM_SFLASH_PERIPHERAL_ID (0)
#endif

#ifndef SFLASH_SECTOR_SIZE
#  define SFLASH_SECTOR_SIZE (4096)
#endif

// Default to using the OTA_APP scratch space in the external SPI flash to store the coredump data:
#ifndef MEMFAULT_PLATFORM_COREDUMP_WICED_DCT_APP_INDEX
#  define MEMFAULT_PLATFORM_COREDUMP_WICED_DCT_APP_INDEX (DCT_OTA_APP_INDEX)
#endif

//! Default storage size to allocate for storing coredump data.
//! Ensure to allocate a bit of extra headroom for metadata.
#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_SIZE_BYTES
#  define MEMFAULT_PLATFORM_COREDUMP_STORAGE_SIZE_BYTES (130 * 1024)
#endif

#ifndef MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY
#  define MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY (0)
#endif

// These symbols are defined in the WICED SDK linker scripts:
extern uint32_t link_stack_location;
extern uint32_t link_stack_end;

// These symbols are defined in the memfault_platform_coredump.ld linker script:
extern uint32_t __MfltCoredumpRamStart;
extern uint32_t __MfltCoredumpRamEnd;

typedef struct sMemfaultPlatformCoredumpCtx {
  // Physical flash addresses:
  uint32_t flash_start;
  uint32_t flash_end;
  uint32_t crc;
} sMemfaultPlatformCoredumpCtx;

static sMemfaultPlatformCoredumpCtx *s_memfault_platform_coredump_ctx;

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                                                  size_t *num_regions) {
  // Let's collect the callstack at the time of crash

  static sMfltCoredumpRegion s_coredump_regions[1];

#if (MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY == 1)
  // Capture only the interrupt stack. Use only if there is not enough storage to capture all of RAM.
  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&link_stack_location, link_stack_size);
#else
  // Capture all of RAM. Recommended: it enables broader post-mortem analyses, but has larger storage requirements.
  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&__MfltCoredumpRamStart,
      (uint32_t)&__MfltCoredumpRamEnd - (uint32_t)&__MfltCoredumpRamStart);
#endif

  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);

  return s_coredump_regions;
}

static wiced_result_t prv_init(void) {
  wiced_app_t app;
  WICED_VERIFY(wiced_framework_app_open(MEMFAULT_PLATFORM_COREDUMP_WICED_DCT_APP_INDEX, &app));

  uint32_t current_size = 0;
  WICED_VERIFY(wiced_framework_app_get_size(&app, &current_size));

  // Resize the SPI flash partition (only if needed):
  if (current_size < MEMFAULT_PLATFORM_COREDUMP_STORAGE_SIZE_BYTES) {
    WICED_VERIFY(wiced_framework_app_set_size(&app, MEMFAULT_PLATFORM_COREDUMP_STORAGE_SIZE_BYTES));
  }

  sflash_handle_t sflash_handle;
  app_header_t    app_header;
  const image_location_t *const app_header_location = &app.app_header_location;
  WICED_VERIFY(init_sflash(&sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED));
  WICED_VERIFY(sflash_read(&sflash_handle, app_header_location->detail.external_fixed.location, &app_header, sizeof(app_header_t)));
  WICED_VERIFY(deinit_sflash(&sflash_handle));

  if (app_header.count != 1) {
    MEMFAULT_LOG_ERROR("Coredump partition must be contiguous! %d parts found", app_header.count);
    return WICED_ERROR;
  }

  // Let's heap-allocate this. Stacks run downwards on ARM. The stacks generally sit below the heap (unless they're
  // allocated on the heap...) In case of a really bad stack-overflow, the data will have a higher likelihood of
  // surviving when living on the heap:
  s_memfault_platform_coredump_ctx = malloc(sizeof(*s_memfault_platform_coredump_ctx));
  if (!s_memfault_platform_coredump_ctx) {
    return WICED_ERROR;
  }
  // Calculate the physical SPI flash addresses:
  const uint32_t flash_start = SFLASH_SECTOR_SIZE * app_header.sectors[0].start;
  const uint32_t flash_end = flash_start + (SFLASH_SECTOR_SIZE * app_header.sectors[0].count);
  const uint32_t actual_size = flash_end - flash_start;
  if (actual_size < MEMFAULT_PLATFORM_COREDUMP_STORAGE_SIZE_BYTES) {
    MEMFAULT_LOG_ERROR("WICED set size failed? %"PRIu32, actual_size);
    return WICED_ERROR;
  }
  *s_memfault_platform_coredump_ctx = (sMemfaultPlatformCoredumpCtx) {
      .flash_start = flash_start,
      .flash_end = flash_end,
  };
  // CRC32 to protect it, just in case the data get clobbered:
  s_memfault_platform_coredump_ctx->crc = memfault_platform_crc32(
      s_memfault_platform_coredump_ctx, offsetof(sMemfaultPlatformCoredumpCtx, crc));
  return WICED_SUCCESS;
}

bool memfault_platform_coredump_boot(void) {
  return (WICED_SUCCESS == prv_init());
}

static bool prv_get_start_and_end_addr(uint32_t *flash_start, uint32_t *flash_end) {
  const uint32_t actual_crc = memfault_platform_crc32(
      s_memfault_platform_coredump_ctx, offsetof(sMemfaultPlatformCoredumpCtx, crc));
  if (actual_crc != s_memfault_platform_coredump_ctx->crc) {
    return false;
  }
  *flash_start = s_memfault_platform_coredump_ctx->flash_start;
  *flash_end = s_memfault_platform_coredump_ctx->flash_end;
  return true;
}

bool memfault_platform_get_spi_start_and_end_addr(uint32_t *flash_start, uint32_t *flash_end) {
  return prv_get_start_and_end_addr(flash_start, flash_end);
}

static bool prv_offset_to_addr(uint32_t offset, size_t read_len, uint32_t *addr_out) {
  uint32_t flash_start;
  uint32_t flash_end;
  if (!prv_get_start_and_end_addr(&flash_start, &flash_end)) {
    return false; // bad crc
  }
  const uint32_t start_addr = flash_start + offset;
  if (start_addr + read_len > flash_end) {
    return false;
  }
  *addr_out = start_addr;
  return true;
}

void memfault_platform_coredump_storage_clear(void) {
  const uint32_t zero = 0;  // Assuming NOR flash!
  memfault_platform_coredump_storage_write(0, &zero, sizeof(zero));
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  uint32_t flash_start;
  uint32_t flash_end;
  uint32_t size = 0;
  if (prv_get_start_and_end_addr(&flash_start, &flash_end)) {
    size = flash_end - flash_start;
  }

  *info  = (sMfltCoredumpStorageInfo) {
    .size = size,
    .sector_size = SFLASH_SECTOR_SIZE,
  };
}

static wiced_result_t prv_write(uint32_t addr, const void *data, size_t data_len) {
  sflash_handle_t sflash_handle;
  WICED_VERIFY(init_sflash(&sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED));
  WICED_VERIFY(sflash_write(&sflash_handle, addr, data, data_len));
  WICED_VERIFY(deinit_sflash(&sflash_handle));
  return WICED_SUCCESS;
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  uint32_t addr;
  if (!prv_offset_to_addr(offset, data_len, &addr)) {
    return false;
  }
  return (WICED_SUCCESS == prv_write(addr, data, data_len));
}

static wiced_result_t prv_read(uint32_t addr, void *data, size_t read_len) {
  sflash_handle_t sflash_handle;
  WICED_VERIFY(init_sflash(&sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED));
  WICED_VERIFY(sflash_read(&sflash_handle, addr, data, read_len));
  WICED_VERIFY(deinit_sflash(&sflash_handle));
  return WICED_SUCCESS;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  uint32_t addr;
  if (!prv_offset_to_addr(offset, read_len, &addr)) {
    return false;
  }
  return (WICED_SUCCESS == prv_read(addr, data, read_len));
}

static wiced_result_t prv_erase(uint32_t addr, size_t erase_size) {
  sflash_handle_t sflash_handle;
  WICED_VERIFY(init_sflash(&sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED));
  for (uint32_t sector_base = addr; sector_base < addr + erase_size; sector_base += SFLASH_SECTOR_SIZE) {
    WICED_VERIFY(sflash_sector_erase(&sflash_handle, sector_base));
  }
  WICED_VERIFY(deinit_sflash(&sflash_handle));
  return WICED_SUCCESS;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  uint32_t addr;
  if (erase_size % SFLASH_SECTOR_SIZE != 0) {
    return false;
  }
  if (!prv_offset_to_addr(offset, erase_size, &addr)) {
    return false;
  }

  // Feed the watchdog since erases can take a little bit to complete
  platform_watchdog_kick();
  return (WICED_SUCCESS == prv_erase(addr, erase_size));
}
