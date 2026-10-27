//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! ESP32 SPI flash backed coredump storage implementation.
//!
//! The Zephyr ESP32 flash driver (and the OS-aware hal_espressif flash guard
//! functions it installs) take semaphores/mutexes, which can't be used from
//! fault context. Instead, this calls the hal_espressif esp_flash_* API
//! directly. When interrupts are locked (fault context, storage self-test), a
//! private copy of the default chip handle is used with the "no-OS" guard
//! functions (cache suspend only, no locking), the same approach the ESP-IDF
//! port uses at panic time.
//!
//! Coredump data is always stored raw (not encrypted), even when flash
//! encryption is enabled.
//!
//! The storage location is either:
//!
//! 1. A dedicated partition named "memfault_coredump_partition" (default):
//!
//!    &flash0 {
//!      partitions {
//!        memfault_coredump_partition: partition@3c0000 {
//!          label = "memfault_coredump_partition";
//!          reg = <0x3c0000 DT_SIZE_K(128)>;
//!        };
//!      };
//!    };
//!
//! 2. The tail of the MCUboot secondary slot (slot1_partition), when
//!    CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT=y. This
//!    permits adding flash coredump storage to deployed devices via an OTA
//!    update without changing the partition table. The region sits just below
//!    the MCUboot trailer, and saving is skipped if an image in the slot
//!    overlaps it.

#include <esp_flash.h>
#include <esp_flash_chips/esp_flash_types.h>
#include <memfault/components.h>
#include <memfault/ports/zephyr/version.h>
#include <zephyr/irq.h>
#include <zephyr/storage/flash_map.h>

#if MEMFAULT_ZEPHYR_VERSION_GTE_STRICT(4, 4)
  #define MEMFAULT_PARTITION_EXISTS(label) PARTITION_EXISTS(label)
  #define MEMFAULT_PARTITION_OFFSET(label) PARTITION_OFFSET(label)
  #define MEMFAULT_PARTITION_SIZE(label) PARTITION_SIZE(label)
#else
  #define MEMFAULT_PARTITION_EXISTS(label) FIXED_PARTITION_EXISTS(label)
  #define MEMFAULT_PARTITION_OFFSET(label) FIXED_PARTITION_OFFSET(label)
  #define MEMFAULT_PARTITION_SIZE(label) FIXED_PARTITION_SIZE(label)
#endif

#define MEMFAULT_ESP32_FLASH_SECTOR_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)

#if defined(CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT)

  #if !MEMFAULT_PARTITION_EXISTS(slot1_partition)
    #error "MCUboot secondary slot partition 'slot1_partition' not found"
  #endif

  #define MEMFAULT_SLOT1_OFFSET MEMFAULT_PARTITION_OFFSET(slot1_partition)
  #define MEMFAULT_SLOT1_SIZE MEMFAULT_PARTITION_SIZE(slot1_partition)
  #define MEMFAULT_COREDUMP_STORAGE_SIZE \
    CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT_SIZE
  #define MEMFAULT_COREDUMP_STORAGE_OFFSET                                          \
    (MEMFAULT_SLOT1_OFFSET + MEMFAULT_SLOT1_SIZE - MEMFAULT_COREDUMP_STORAGE_SIZE - \
     CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_TRAILER_RESERVE)

MEMFAULT_STATIC_ASSERT(CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_TRAILER_RESERVE %
                           MEMFAULT_ESP32_FLASH_SECTOR_SIZE ==
                         0,
                       "MCUboot trailer reserve must be a multiple of the flash sector size");
MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_STORAGE_SIZE +
                           CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_TRAILER_RESERVE <
                         MEMFAULT_SLOT1_SIZE,
                       "Coredump region does not fit in the MCUboot secondary slot");

#else

  #if !MEMFAULT_PARTITION_EXISTS(memfault_coredump_partition)
    #error "Be sure to add a fixed partition named 'memfault_coredump_partition'!"
  #endif

  #define MEMFAULT_COREDUMP_STORAGE_OFFSET MEMFAULT_PARTITION_OFFSET(memfault_coredump_partition)
  #define MEMFAULT_COREDUMP_STORAGE_SIZE MEMFAULT_PARTITION_SIZE(memfault_coredump_partition)

#endif

MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_STORAGE_OFFSET % MEMFAULT_ESP32_FLASH_SECTOR_SIZE == 0,
                       "Coredump storage must be sector-aligned");
MEMFAULT_STATIC_ASSERT(MEMFAULT_COREDUMP_STORAGE_SIZE % MEMFAULT_ESP32_FLASH_SECTOR_SIZE == 0,
                       "Coredump storage size must be a multiple of the flash sector size");

// Not in a public header; defined in hal_espressif spi_flash_os_func_noos.c
extern const esp_flash_os_functions_t esp_flash_noos_functions;

// Must be in internal RAM: it's accessed by the flash HAL while cache is suspended
static esp_flash_t s_noos_chip;

//! The OS-aware guard functions block on a mutex/semaphore, so they can only be
//! used with interrupts enabled. The no-OS ones don't block other code running
//! from flash, so they can only be used with interrupts locked.
static esp_flash_t *prv_get_chip(void) {
  const unsigned int key = irq_lock();
  irq_unlock(key);
  if (arch_irq_unlocked(key)) {
    return NULL;  // default chip
  }

  s_noos_chip = *esp_flash_default_chip;
  s_noos_chip.os_func = &esp_flash_noos_functions;
  s_noos_chip.os_func_data = NULL;
  return &s_noos_chip;
}

static bool prv_op_within_bounds(uint32_t offset, size_t len) {
  return (offset <= MEMFAULT_COREDUMP_STORAGE_SIZE) &&
         (len <= MEMFAULT_COREDUMP_STORAGE_SIZE - offset);
}

static bool prv_flash_read(uint32_t address, void *data, size_t len) {
  return esp_flash_read(prv_get_chip(), data, address, len) == ESP_OK;
}

#if defined(CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT)

// Subset of the MCUboot image format (bootutil/image.h), which is stable
  #define MCUBOOT_IMAGE_MAGIC 0x96f3b83d
  #define MCUBOOT_TLV_INFO_MAGIC 0x6907

typedef MEMFAULT_PACKED_STRUCT {
  uint32_t ih_magic;
  uint32_t ih_load_addr;
  uint16_t ih_hdr_size;
  uint16_t ih_protect_tlv_size;
  uint32_t ih_img_size;
}
sMcubootImageHeader;

typedef MEMFAULT_PACKED_STRUCT {
  uint16_t it_magic;
  uint16_t it_tlv_tot;
}
sMcubootTlvInfo;

//! Returns false if an MCUboot image at hdr_offset (relative to slot start)
//! extends into the coredump region.
static bool prv_slot1_image_fits(uint32_t hdr_offset) {
  sMcubootImageHeader hdr;
  if (!prv_flash_read(MEMFAULT_SLOT1_OFFSET + hdr_offset, &hdr, sizeof(hdr))) {
    return false;
  }
  if (hdr.ih_magic != MCUBOOT_IMAGE_MAGIC) {
    return true;
  }

  const uint32_t region_start = MEMFAULT_COREDUMP_STORAGE_OFFSET - MEMFAULT_SLOT1_OFFSET;
  uint64_t image_end =
    (uint64_t)hdr_offset + hdr.ih_hdr_size + hdr.ih_img_size + hdr.ih_protect_tlv_size;
  if (image_end > region_start) {
    return false;
  }

  sMcubootTlvInfo tlv;
  if (!prv_flash_read(MEMFAULT_SLOT1_OFFSET + (uint32_t)image_end, &tlv, sizeof(tlv))) {
    return false;
  }
  // If the TLV area is missing (eg partial download), assume a sector of TLVs
  image_end +=
    (tlv.it_magic == MCUBOOT_TLV_INFO_MAGIC) ? tlv.it_tlv_tot : MEMFAULT_ESP32_FLASH_SECTOR_SIZE;
  return image_end <= region_start;
}

static bool prv_storage_region_is_free(void) {
  // Swap-using-offset places the secondary image one sector into the slot
  return prv_slot1_image_fits(0) && prv_slot1_image_fits(MEMFAULT_ESP32_FLASH_SECTOR_SIZE);
}

#endif  // CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = (sMfltCoredumpStorageInfo){
    .size = MEMFAULT_COREDUMP_STORAGE_SIZE,
    .sector_size = MEMFAULT_ESP32_FLASH_SECTOR_SIZE,
  };
}

bool memfault_port_coredump_save_begin(void) {
  // Selects the no-OS flash functions for the save. Never unlocked: the system
  // reboots after the save, and the storage self-test restores its own key.
  (void)irq_lock();

#if defined(CONFIG_MEMFAULT_COREDUMP_STORAGE_ESP32_FLASH_MCUBOOT_SECONDARY_SLOT)
  // Don't clobber a downloaded update or revert image
  return prv_storage_region_is_free();
#else
  return true;
#endif
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  if (!prv_op_within_bounds(offset, read_len)) {
    return false;
  }
  return prv_flash_read(MEMFAULT_COREDUMP_STORAGE_OFFSET + offset, data, read_len);
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_bounds(offset, erase_size)) {
    return false;
  }
  return esp_flash_erase_region(prv_get_chip(), MEMFAULT_COREDUMP_STORAGE_OFFSET + offset,
                                erase_size) == ESP_OK;
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data, size_t data_len) {
  if (!prv_op_within_bounds(offset, data_len)) {
    return false;
  }
  return esp_flash_write(prv_get_chip(), data, MEMFAULT_COREDUMP_STORAGE_OFFSET + offset,
                         data_len) == ESP_OK;
}

void memfault_platform_coredump_storage_clear(void) {
  // Zeroing the header magic invalidates the coredump without an erase
  const uint32_t invalidate = 0;
  memfault_platform_coredump_storage_write(0, &invalidate, sizeof(invalidate));
}
