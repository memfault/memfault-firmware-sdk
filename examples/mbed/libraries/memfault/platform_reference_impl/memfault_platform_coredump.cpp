//! @file
//! @brief
//! Reference implementation of platform dependency functions which could be used
//! for coredump collection on Mbed OS 5

#include "memfault/panics/platform/coredump.h"

#include <string.h>

#include "hal/flash_api.h"
#include "platform/mbed_mpu_mgmt.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"

#include "mbed.h"

// Linker symbols available in a default mbed configuration:
extern uint32_t _estack;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __data_start__;
extern uint32_t __data_end__;

const uint32_t __MfltCoredumpRamStart = 0x20000000;
const uint32_t __MfltCoredumpRamEnd = (uint32_t)&_estack;

// We'll use the last STM32F429I flash sector to hold a coredump
// The part itself has 192kB of regular SRAM and 64kB of external SRAM
// All of RAM could be captured in a port by allocating more space for storage
#define CORE_REGION_LENGTH (128 * 1024)
#define CORE_REGION_START (0x8000000 + MBED_ROM_SIZE - CORE_REGION_LENGTH)

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  // NOTE: This is just an example of regions which could be collected
  // Depending on your use case, you may also want to consider collecting the mbed heap
  // for example. By default, this runs from __bss_end__ up to the beginning of the ISR stack

  static sMfltCoredumpRegion s_coredump_regions[3];

  const uint32_t bss_length = (uint32_t)&__bss_end__ - (uint32_t)&__bss_start__;
  const uint32_t data_length = (uint32_t)&__data_end__ - (uint32_t)&__data_start__;

  // The ISR stack is allocated at the end of RAM. Let's collect the top 4kB in case the stack
  // runs too far down
  const uint32_t isr_stack_bytes_to_collect = 4096;
  const uint32_t isr_stack_begin_addr = (uint32_t)&_estack - isr_stack_bytes_to_collect;

  s_coredump_regions[0] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      &__bss_start__, bss_length);
  s_coredump_regions[1] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      &__data_start__, data_length);
  s_coredump_regions[2] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      (uint32_t *)isr_stack_begin_addr, isr_stack_bytes_to_collect);


  *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
  return s_coredump_regions;
}

void prv_mpu_unprotect_flash(void) {
  // confusing name; taking lock allows flash write
  mbed_mpu_manager_lock_rom_write();
}

void prv_mpu_protect_flash(void) {
  // confusing name; releasing lock disallows flash write
  mbed_mpu_manager_unlock_rom_write();
}

// Note: This API is the only one which is called while the system is running.  It's typically
// invoked once a coredump has been successfully flushed to the Memfault cloud. Therefore, we use
// the higher level mBed flash API so safely lock around the operation. All the other platform flash
// APIs are called while nothing else is running so we use the lower level flash hal.
void memfault_platform_coredump_storage_clear(void) {
  FlashIAP flash;
  flash.init();
  {
    uint8_t clear_byte_val = ~flash.get_erase_value();
    flash.program(&clear_byte_val,CORE_REGION_START, sizeof(clear_byte_val));
  }
  flash.deinit();
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  flash_t fla;
  int retval = flash_init(&fla);
  if (retval != 0) { return; }

  *info  = (sMfltCoredumpStorageInfo) {
    .size = CORE_REGION_LENGTH,
    .sector_size = flash_get_sector_size(&fla, CORE_REGION_START),
  };

  flash_free(&fla);
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > CORE_REGION_LENGTH) {
    return false;
  }

  flash_t fla;
  int retval = flash_init(&fla);
  if (retval != 0) { return false; }

  uint32_t address = CORE_REGION_START + offset;
  prv_mpu_unprotect_flash();
  retval = flash_program_page(&fla, address, (const uint8_t *)data, data_len /* bytes */);
  prv_mpu_protect_flash();

  flash_free(&fla);
  return retval == 0;
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {

  if ((offset + read_len) > CORE_REGION_LENGTH) {
    return false;
  }

  flash_t fla;
  int retval = flash_init(&fla);
  if (retval != 0) { return false; }

  uint32_t address = CORE_REGION_START + offset;
  retval = flash_read(&fla, address, (uint8_t *)data, read_len);

  flash_free(&fla);
  return retval == 0;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > CORE_REGION_LENGTH) {
    return false;
  }

  flash_t fla;
  int retval = flash_init(&fla);
  if (retval != 0) { return false; }

  const size_t sector_size = flash_get_sector_size(&fla, CORE_REGION_START);
  if ((offset % sector_size) != 0) {
    flash_free(&fla);
    return false;
  }

  for (size_t sector = offset; sector < erase_size; sector += sector_size) {
    uint32_t address = CORE_REGION_START + sector;

    prv_mpu_unprotect_flash();
    retval = flash_erase_sector(&fla, address);
    prv_mpu_protect_flash();

    if (retval != 0) { break; }
  }

  flash_free(&fla);
  return retval == 0;
}
