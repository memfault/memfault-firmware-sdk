//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Logic for saving a coredump to backing storage and reading it out

#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"

#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/core/platform/device_info.h"

#define MEMFAULT_COREDUMP_MAGIC 0x45524f43
#define MEMFAULT_COREDUMP_VERSION 1

typedef struct MEMFAULT_PACKED MfltCoredumpHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t total_size;
  uint8_t data[];
} sMfltCoredumpHeader;

typedef struct MEMFAULT_PACKED MfltCoredumpBlock {
  eMfltCoredumpBlockType block_type:8;
  uint8_t rsvd[3];
  uint32_t address;
  uint32_t length;
} sMfltCoredumpBlock;

typedef struct MEMFAULT_PACKED MfltTraceReasonBlock {
  uint32_t reason;
} sMfltTraceReasonBlock;

// Using ELF machine enum values:
// https://refspecs.linuxfoundation.org/elf/gabi4%2B/ch4.eheader.html
typedef enum MfltCoredumpMachineType  {
  kMfltCoredumpMachineType_None = 0,
  kMfltCoredumpMachineType_ARM = 40,
  kMfltCoredumpMachineType_Xtensa = 94,
} eMfltCoredumpMachineType;

typedef union MEMFAULT_PACKED MfltMachineTypeBlock {
  eMfltCoredumpMachineType machine_type;
  uint32_t u32;
} sMfltMachineTypeBlock;

static bool prv_write_storage(const void *data, size_t len, void *ctx) {
  uint32_t *offset = ctx;
  if (!memfault_platform_coredump_storage_write(*offset, data, len)) {
    return false;
  }
  *offset += len;
  return true;
}

static bool prv_write_block_with_address(eMfltCoredumpBlockType block_type,
    const void *block_payload, size_t block_payload_size, uint32_t address,
    MfltCoredumpWriteCb write_cb, void *ctx) {
  const sMfltCoredumpBlock blk = {
      .block_type = block_type,
      .address = address,
      .length = block_payload_size,
  };
  if (!write_cb(&blk, sizeof(blk), ctx)) {
    return false;
  }
  // When NULL is passed as block_payload, only the header is written, even if the payload size is non-zero, this
  // is a feature that allows one to write the block payload piece-meal.
  if (block_payload_size && block_payload) {
    if (!write_cb(block_payload, block_payload_size, ctx)) {
      return false;
    }
  }
  return true;
}

bool memfault_coredump_write_block(eMfltCoredumpBlockType block_type,
                                   const void *block_payload, size_t block_payload_size,
                                   MfltCoredumpWriteCb write_cb, void *ctx) {
  return prv_write_block_with_address(block_type, block_payload, block_payload_size, 0, write_cb, ctx);
}

static eMfltCoredumpBlockType prv_region_type_to_storage_type(eMfltCoredumpRegionType type) {
  switch (type) {
    case kMfltCoredumpRegionType_ImageIdentifier:
    case kMfltCoredumpRegionType_Memory:
    default:
      return kMfltCoredumpBlockType_MemoryRegion;
  }
}

static eMfltCoredumpMachineType prv_get_machine_type(void) {
#if defined(MEMFAULT_UNITTEST)
  return kMfltCoredumpMachineType_None;
#else
#  if defined(__arm__)
  return kMfltCoredumpMachineType_ARM;
#  elif defined(__XTENSA__)
  return kMfltCoredumpMachineType_Xtensa;
#  else
  return kMfltCoredumpMachineType_None;
#  endif
#endif
}

void memfault_coredump_write_device_info_blocks(MfltCoredumpWriteCb write_cb, void *ctx) {
  struct MemfaultDeviceInfo info;
  memfault_platform_get_device_info(&info);

  if (info.device_serial) {
    memfault_coredump_write_block(kMfltCoredumpRegionType_DeviceSerial,
        info.device_serial, strlen(info.device_serial), write_cb, ctx);
  }

  if (info.fw_version) {
    memfault_coredump_write_block(kMfltCoredumpRegionType_FirmwareVersion,
        info.fw_version, strlen(info.fw_version), write_cb, ctx);
  }

  if (info.hardware_version) {
    memfault_coredump_write_block(kMfltCoredumpRegionType_HardwareRevision,
        info.hardware_version, strlen(info.hardware_version), write_cb, ctx);
  }

  const sMfltMachineTypeBlock machine_block = {
      .machine_type = prv_get_machine_type(),
  };
  memfault_coredump_write_block(kMfltCoredumpRegionType_MachineType,
      &machine_block, sizeof(machine_block), write_cb, ctx);
}

bool memfault_coredump_write_header(size_t total_coredump_size,
                                    MfltCoredumpWriteCb write_cb, void *ctx) {
  sMfltCoredumpHeader hdr = (sMfltCoredumpHeader) {
      .magic = MEMFAULT_COREDUMP_MAGIC,
      .version = MEMFAULT_COREDUMP_VERSION,
      .total_size = total_coredump_size,
  };
  return write_cb(&hdr, sizeof(hdr), ctx);
}

static void prv_try_write_trace_reason(uint32_t *offset, uint32_t trace_reason) {
  sMfltTraceReasonBlock trace_info = {
    .reason = trace_reason,
  };

  memfault_coredump_write_block(kMfltCoredumpRegionType_TraceReason,
      &trace_info, sizeof(trace_info), prv_write_storage, offset);
}

// When copying out some regions (for example, memory or register banks)
// we want to make sure we can do word-aligned accesses.
static void prv_insert_padding_if_necessary(uint32_t *offset) {
  const size_t word_size = 4;
  const size_t remainder = *offset % word_size;
  if (remainder == 0) {
    return;
  }

  size_t padding_needed = word_size - remainder;
  uint8_t pad_bytes[padding_needed];
  memset(pad_bytes, 0x0, sizeof(pad_bytes));

  memfault_coredump_write_block(kMfltCoredumpRegionType_PaddingRegion,
      &pad_bytes, sizeof(pad_bytes), prv_write_storage, offset);
}

static bool prv_get_info_and_header(sMfltCoredumpHeader *hdr_out, sMfltCoredumpStorageInfo *info_out) {
  sMfltCoredumpStorageInfo info = { 0 };
  memfault_platform_coredump_storage_get_info(&info);
  if (info.size == 0) {
    return false; // no space for core files!
  }

  if (!memfault_platform_coredump_storage_read(0, hdr_out, sizeof(*hdr_out))) {
    return false; // read failure, abort!
  }

  if (info_out) {
    *info_out = info;
  }
  return true;
}

static bool prv_coredump_get_header(sMfltCoredumpHeader *hdr_out) {
  return prv_get_info_and_header(hdr_out, NULL);
}

static bool prv_coredump_header_is_valid(const sMfltCoredumpHeader *hdr) {
  return (hdr && hdr->magic == MEMFAULT_COREDUMP_MAGIC);
}

static bool prv_write_regions(uint32_t *curr_offset, const sMfltCoredumpRegion *regions,
                              size_t num_regions) {
  for (size_t i = 0; i < num_regions; i++) {
    prv_insert_padding_if_necessary(curr_offset);
    const sMfltCoredumpRegion *region = &regions[i];
    if (!prv_write_block_with_address(prv_region_type_to_storage_type(region->type),
                                      region->region_start, region->region_size, (uint32_t)(uintptr_t)region->region_start,
                                      prv_write_storage, curr_offset)) {
      return false;
    }
  }
  return true;
}

void memfault_coredump_save(void *regs, size_t size, uint32_t trace_reason) {
  sMfltCoredumpStorageInfo info = { 0 };
  sMfltCoredumpHeader hdr = { 0 };
  if (!prv_get_info_and_header(&hdr, &info)) {
    return;
  }

  if (prv_coredump_header_is_valid(&hdr)) {
    return; // don't overwrite what we got!
  }

  // are there some regions for us to write?
  size_t num_regions;
  const sMfltCoredumpRegion *regions = memfault_platform_coredump_get_regions(&num_regions);
  if ((regions == NULL) || (num_regions == 0)) {
    // sanity check that we got something valid from the caller
    return;
  }

  // MAYBE: We could have an additional check here to ensure that the platform has
  // given us enough storage space to save the number of regions requested

  // Erase whatever we got
  if (!memfault_platform_coredump_storage_erase(0, info.size)) {
    return;
  }

  // We will write the header last as a way to mark validity
  uint32_t curr_offset = sizeof(hdr);

  if (regs != NULL) {
    if (!memfault_coredump_write_block(kMfltCoredumpBlockType_CurrentRegisters,
        regs, size, prv_write_storage, &curr_offset)) {
      return;
    }
  }

  memfault_coredump_write_device_info_blocks(prv_write_storage, &curr_offset);

  prv_try_write_trace_reason(&curr_offset, trace_reason);

  // write out any architecture specific regions
  size_t num_arch_regions;
  const sMfltCoredumpRegion *arch_regions = memfault_coredump_get_arch_regions(&num_arch_regions);
  if (!prv_write_regions(&curr_offset, arch_regions, num_arch_regions)) {
    return;
  }

  if (!prv_write_regions(&curr_offset, regions, num_regions)) {
    return;
  }

  // we are done, mark things as valid
  size_t header_offset = 0;
  memfault_coredump_write_header(curr_offset, prv_write_storage, &header_offset);
}

bool memfault_coredump_has_valid_coredump(size_t *total_size_out) {
  sMfltCoredumpHeader hdr = { 0 };
  if (!prv_coredump_get_header(&hdr)) {
    return false;
  }
  if (!prv_coredump_header_is_valid(&hdr)) {
    return false;
  }
  if (total_size_out) {
    *total_size_out = hdr.total_size;
  }
  return true;
}
