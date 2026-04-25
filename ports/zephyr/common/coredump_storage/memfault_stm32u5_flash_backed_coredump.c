//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! STM32U5 Flash backed coredump storage implementation using direct SOC flash API.
//! The Zephyr flash API uses semaphores which cannot be used in ISR context,
//! so this implementation directly accesses the STM32 flash controller registers.
//!
//! To make use of this, be sure to add a fixed partition named
//! "memfault_coredump_partition" to your board's device tree:
//!
//!  &flash0 {
//!    partitions {
//!      compatible = "fixed-partitions";
//!      #address-cells = <1>;
//!      #size-cells = <1>;
//!
//!      memfault_coredump_partition: partition@f0000 {
//!        label = "memfault_coredump_partition";
//!        reg = <0xf0000 0x10000>;
//!      };
//!    };
//!  };

#include <memfault/components.h>
#include <stm32_ll_icache.h>
#include <stm32u5xx_ll_dcache.h>
#include <stm32u5xx_ll_system.h>
#include <zephyr/cache.h>
#include <zephyr/storage/flash_map.h>

#include "memfault/ports/zephyr/version.h"

//! STM32U5 flash has 8KB pages and 16-byte (quadword) write block size
#define MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE (16)
#include <memfault/ports/buffered_coredump_storage.h>

// The CMSIS device headers (included transitively via stm32u5xx_ll_system.h) define the
// flash register bit fields for STM32U5X. These are distinct from the STM32L5 naming:
//
//   NSCR: LOCK(31) | ... | STRT(16) | ... | BKER(11) | PNB(9:3) | ... | PER(1) | PG(0)
//   NSSR: ... | BSY(16) | OPTWERR(13) | PGSERR(7) | SIZERR(6) | PGAERR(5) | WRPERR(4) |
//              PROGERR(3) | OPERR(1) | EOP(0)
//   OPTR: DUALBANK(21) | SWAP_BANK(20) | ...
//
// Catch wrong-target builds early.
#if !defined(FLASH_NSCR_LOCK) || !defined(FLASH_NSSR_BSY) || !defined(FLASH_OPTR_DUALBANK)
  #error "Expected STM32U5 CMSIS defines not found. Wrong target or missing includes."
#endif

// Ensure the memfault_coredump_partition entry exists
#if !FIXED_PARTITION_EXISTS(memfault_coredump_partition)
  #error "Be sure to add a fixed partition named 'memfault_coredump_partition'!"
#endif

// FIXED_PARTITION_ADDRESS returns the absolute memory-mapped address
// (flash base + partition offset), so no arithmetic needed here.

#if !MEMFAULT_ZEPHYR_VERSION_GTE_STRICT(4, 3)
  #define FIXED_PARTITION_ADDRESS(label)                            \
    (COND_CODE_1(DT_FIXED_SUBPARTITION_EXISTS(DT_NODELABEL(label)), \
                 (DT_FIXED_SUBPARTITION_ADDR(DT_NODELABEL(label))), \
                 (DT_FIXED_PARTITION_ADDR(DT_NODELABEL(label)))))
#endif

#define MEMFAULT_COREDUMP_PARTITION_ADDRESS FIXED_PARTITION_ADDRESS(memfault_coredump_partition)
#define MEMFAULT_COREDUMP_PARTITION_SIZE FIXED_PARTITION_SIZE(memfault_coredump_partition)

MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_PARTITION_SIZE % MEMFAULT_COREDUMP_STORAGE_WRITE_SIZE == 0,
                       "Storage size must be a multiple of 16 bytes (quadword)");
MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_PARTITION_ADDRESS % FLASH_PAGE_SIZE == 0,
                       "Coredump partition address must be page-aligned");
MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_PARTITION_SIZE % FLASH_PAGE_SIZE == 0,
                       "Coredump partition size must be a multiple of FLASH_PAGE_SIZE");

// Flash base address derived from the device tree.
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_INST(0, st_stm32_nv_flash))

// Offset from flash base where bank 2 begins in dual-bank mode.
#define FLASH_BANK2_OFFSET (CONFIG_FLASH_SIZE * 1024 / 2)

// NSSR error bits to check and clear. These are all the sticky error flags
// in the non-secure status register for STM32U5X.
#define FLASH_NSSR_ERRORS                                                          \
  (FLASH_NSSR_OPERR | FLASH_NSSR_PROGERR | FLASH_NSSR_WRPERR | FLASH_NSSR_PGAERR | \
   FLASH_NSSR_SIZERR | FLASH_NSSR_PGSERR | FLASH_NSSR_OPTWERR)

// Standard STM32 flash unlock key sequence (same across all STM32 families).
#define FLASH_KEY1_VALUE (0x45670123UL)
#define FLASH_KEY2_VALUE (0xCDEF89ABUL)

// Maximum time to wait for a flash operation. STM32U5 typical page erase is ~90 ms.
#define FLASH_TIMEOUT_US (200000UL)

// Rough microsecond busy-wait. Only used as a watchdog; the BSY flag is the
// real completion signal. The STM32U5 can run up to 160 MHz — the loop body
// takes roughly 2 cycles, so 160/2 = 80 iterations ≈ 1 µs at max frequency.
// At lower frequencies the wait will be longer, which is conservative and fine.
static inline void prv_delay_us(uint32_t us) {
  volatile uint32_t cycles = us * 80;
  while (cycles--) {
    __asm__ volatile("nop");
  }
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo){
    .size = MEMFAULT_COREDUMP_PARTITION_SIZE,
    .sector_size = FLASH_PAGE_SIZE,
  };
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sMfltCoredumpStorageInfo info = { 0 };
  memfault_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

static inline bool prv_flash_is_busy(void) {
  // Only check BSY (bit 16 of NSSR for STM32U5X), matching the Zephyr STM32 flash
  // driver. NSWBNE / WDW can stay set briefly after BSY clears; polling it would
  // cause spurious timeouts.
  return (FLASH->NSSR & FLASH_NSSR_BSY) != 0;
}

static bool prv_wait_flash_idle(void) {
  uint32_t timeout_us = FLASH_TIMEOUT_US;

  while (prv_flash_is_busy() && timeout_us > 0) {
    prv_delay_us(1);
    timeout_us--;
  }

  if (FLASH->NSSR & FLASH_NSSR_ERRORS) {
    FLASH->NSSR = FLASH_NSSR_ERRORS;  // clear sticky error bits by writing 1s
    return false;
  }

  return timeout_us > 0;
}

static bool prv_flash_unlock(void) {
  // NSCR_LOCK is bit 31 for STM32U5X (set = locked, clear = unlocked).
  if ((FLASH->NSCR & FLASH_NSCR_LOCK) == 0) {
    return true;
  }

  FLASH->NSKEYR = FLASH_KEY1_VALUE;
  FLASH->NSKEYR = FLASH_KEY2_VALUE;

  return (FLASH->NSCR & FLASH_NSCR_LOCK) == 0;
}

static void prv_flash_lock(void) {
  FLASH->NSCR |= FLASH_NSCR_LOCK;
}

//! Erase the 8 KB page that contains @p flash_address.
//!
//! @p flash_address must be the absolute memory-mapped address (0x08xxxxxx).
//! icache must be disabled by the caller.
static bool prv_erase_page(uint32_t flash_address) {
  if (!prv_wait_flash_idle()) {
    return false;
  }

  if (!prv_flash_unlock()) {
    return false;
  }

  // Work in terms of offset-from-flash-base for bank/page arithmetic, matching
  // the logic in Zephyr's flash_stm32l5x driver.
  const uint32_t offset = flash_address - FLASH_BASE_ADDRESS;

  const bool dual_bank = (FLASH->OPTR & FLASH_OPTR_DUALBANK) != 0;
  const bool bank_swap = (FLASH->OPTR & FLASH_OPTR_SWAP_BANK) != 0;

  uint32_t page;

  if (dual_bank) {
    // Four cases: (physical bank 1 or 2) × (swap active or not).
    // With SWAP_BANK=0: bank 1 occupies [0, BANK2_OFFSET), bank 2 the rest.
    // With SWAP_BANK=1: the logical mapping is flipped.
    if ((offset < FLASH_BANK2_OFFSET) && !bank_swap) {
      // Physically bank 1, no swap
      FLASH->NSCR &= ~FLASH_NSCR_BKER_Msk;
      page = offset / FLASH_PAGE_SIZE;
    } else if ((offset >= FLASH_BANK2_OFFSET) && bank_swap) {
      // Physically beyond BANK2_OFFSET, but swap makes it bank 1
      FLASH->NSCR &= ~FLASH_NSCR_BKER_Msk;
      page = (offset - FLASH_BANK2_OFFSET) / FLASH_PAGE_SIZE;
    } else if ((offset < FLASH_BANK2_OFFSET) && bank_swap) {
      // Physically below BANK2_OFFSET, but swap makes it bank 2
      FLASH->NSCR |= FLASH_NSCR_BKER;
      page = offset / FLASH_PAGE_SIZE;
    } else {
      // offset >= BANK2_OFFSET, no swap → bank 2
      FLASH->NSCR |= FLASH_NSCR_BKER;
      page = (offset - FLASH_BANK2_OFFSET) / FLASH_PAGE_SIZE;
    }
  } else {
    // Single-bank: pages numbered from 0 across the full flash
    FLASH->NSCR &= ~FLASH_NSCR_BKER_Msk;
    page = offset / FLASH_PAGE_SIZE;
  }

  // Configure and start page erase
  FLASH->NSCR |= FLASH_NSCR_PER;
  FLASH->NSCR &= ~FLASH_NSCR_PNB_Msk;
  FLASH->NSCR |= (page << FLASH_NSCR_PNB_Pos);
  FLASH->NSCR |= FLASH_NSCR_STRT;

  // Flush the register pipeline before polling BSY
  volatile uint32_t tmp = FLASH->NSCR;
  (void)tmp;

  const bool result = prv_wait_flash_idle();

  // Clear erase-mode bits
  if (dual_bank) {
    FLASH->NSCR &= ~(FLASH_NSCR_PER | FLASH_NSCR_BKER);
  } else {
    FLASH->NSCR &= ~FLASH_NSCR_PER;
  }

  prv_flash_lock();

  return result;
}

static bool prv_write_quadword(uint32_t flash_address, const uint32_t *data) {
  if (!prv_wait_flash_idle()) {
    return false;
  }

  // Per RM0456 §7.3.7: writing a non-zero quadword over a location that has
  // not been erased (all 0xFF) is illegal. Writing all-zeros is always allowed
  // (programming 1→0 is fine; only 0→1 requires an erase).
  volatile uint32_t *flash_ptr = (volatile uint32_t *)flash_address;
  bool all_zeros = true;
  for (int i = 0; i < 4; i++) {
    if (data[i] != 0) {
      all_zeros = false;
      break;
    }
  }
  if (!all_zeros) {
    for (int i = 0; i < 4; i++) {
      if (flash_ptr[i] != 0xFFFFFFFFUL) {
        return false;  // destination not erased
      }
    }
  }

  if (!prv_flash_unlock()) {
    return false;
  }

  FLASH->NSCR |= FLASH_NSCR_PG;

  // Flush the register write before touching flash
  volatile uint32_t tmp = FLASH->NSCR;
  (void)tmp;

  for (int i = 0; i < 4; i++) {
    flash_ptr[i] = data[i];
  }

  const bool result = prv_wait_flash_idle();

  FLASH->NSCR &= ~FLASH_NSCR_PG;

  prv_flash_lock();

  return result;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  // Flash is memory-mapped so we can read directly
  memcpy(data, (const void *)(MEMFAULT_COREDUMP_PARTITION_ADDRESS + offset), read_len);
  return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  // Per memfault/panics/platform/coredump.h, offset must be sector-aligned and
  // erase_size must be a multiple of sector_size. Refuse unaligned requests to
  // avoid erasing more (or less) than the caller asked for.
  if ((offset % FLASH_PAGE_SIZE) != 0 || (erase_size % FLASH_PAGE_SIZE) != 0) {
    return false;
  }

  // Disable ICACHE and DCACHE1 before any flash operation.
  // Per RM0456 §7.3.10, ICACHE must be disabled during NVM write/erase or ERRF
  // is set in ICACHE_SR and the operation may not complete.  DCACHE1 must be
  // off so data writes reach the flash controller bus (not a write-back line);
  // re-enabling DCACHE1 afterwards triggers an automatic full invalidation
  // (RM0456 §6.3) so subsequent reads see the fresh erased/written data.
  // sys_cache_instr/data_disable are NOT used: on Zephyr < 4.3 both are no-ops
  // for STM32U5 (CONFIG_ICACHE/DCACHE absent), so we call the LL functions
  // directly — matching what Zephyr's own STM32 flash driver does.
  const bool icache_was_enabled = LL_ICACHE_IsEnabled();
  if (icache_was_enabled) {
    CLEAR_BIT(ICACHE->FCR, ICACHE_FCR_CBSYENDF);
    LL_ICACHE_Disable();
    while (LL_ICACHE_IsEnabled()) { }
  }
  const bool dcache_was_enabled = LL_DCACHE_IsEnabled(DCACHE1);
  if (dcache_was_enabled) {
    LL_DCACHE_Disable(DCACHE1);
  }

  bool result = true;
  const uint32_t start_addr = MEMFAULT_COREDUMP_PARTITION_ADDRESS + offset;
  const uint32_t end_addr = start_addr + erase_size;

  for (uint32_t addr = start_addr; addr < end_addr; addr += FLASH_PAGE_SIZE) {
    if (!prv_erase_page(addr)) {
      result = false;
      break;
    }
  }

  if (icache_was_enabled) {
    LL_ICACHE_Enable();
  }
  if (dcache_was_enabled) {
    LL_DCACHE_Enable(DCACHE1);  // re-enable triggers automatic full invalidation
  }

  return result;
}

bool memfault_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  if (!prv_op_within_flash_bounds(blk->write_offset, sizeof(blk->data))) {
    return false;
  }

  const uint32_t addr = MEMFAULT_COREDUMP_PARTITION_ADDRESS + blk->write_offset;

  // Disable ICACHE and DCACHE1 around flash writes (see erase comment above).
  const bool icache_was_enabled = LL_ICACHE_IsEnabled();
  if (icache_was_enabled) {
    CLEAR_BIT(ICACHE->FCR, ICACHE_FCR_CBSYENDF);
    LL_ICACHE_Disable();
    while (LL_ICACHE_IsEnabled()) { }
  }
  const bool dcache_was_enabled = LL_DCACHE_IsEnabled(DCACHE1);
  if (dcache_was_enabled) {
    LL_DCACHE_Disable(DCACHE1);
  }

  const bool result = prv_write_quadword(addr, (const uint32_t *)blk->data);

  if (icache_was_enabled) {
    LL_ICACHE_Enable();
  }
  if (dcache_was_enabled) {
    LL_DCACHE_Enable(DCACHE1);  // re-enable triggers automatic full invalidation
  }

  return result;
}

//! Erase the first page of the coredump partition to invalidate any stored
//! coredump (erased STM32 flash reads back as 0xFF, so any prior coredump
//! header is no longer considered valid).
void memfault_platform_coredump_storage_clear(void) {
  memfault_platform_coredump_storage_erase(0, FLASH_PAGE_SIZE);
}
