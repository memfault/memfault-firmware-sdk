//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Logic for saving a coredump to backing storage and reading it out

#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"

#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/panics/platform/coredump.h"

#define MEMFAULT_COREDUMP_MAGIC 0x45524f43
#define MEMFAULT_COREDUMP_VERSION 1

typedef MEMFAULT_PACKED_STRUCT MfltCoredumpHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t total_size;
  uint8_t data[];
} sMfltCoredumpHeader;

typedef MEMFAULT_PACKED_STRUCT MfltCoredumpBlock {
  eMfltCoredumpBlockType block_type:8;
  uint8_t rsvd[3];
  uint32_t address;
  uint32_t length;
} sMfltCoredumpBlock;

typedef MEMFAULT_PACKED_STRUCT MfltTraceReasonBlock {
  uint32_t reason;
} sMfltTraceReasonBlock;

// Using ELF machine enum values:
// https://refspecs.linuxfoundation.org/elf/gabi4%2B/ch4.eheader.html
typedef enum MfltCoredumpMachineType  {
  kMfltCoredumpMachineType_None = 0,
  kMfltCoredumpMachineType_ARM = 40,
  kMfltCoredumpMachineType_Xtensa = 94,
} eMfltCoredumpMachineType;

typedef MEMFAULT_PACKED_STRUCT MfltMachineTypeBlock {
  uint32_t machine_type;
} sMfltMachineTypeBlock;

static bool prv_write_block_with_address(
    eMfltCoredumpBlockType block_type, const void *block_payload, size_t block_payload_size,
    uint32_t address, MfltCoredumpWriteCb write_cb, void *ctx, bool word_aligned_reads_only) {
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
  if (block_payload_size == 0 || (block_payload == NULL)) {
    return true;
  }

  if (!word_aligned_reads_only || ((block_payload_size % 4) != 0)) {
    // no requirements on how the 'address' is read so whatever the user implementation does is fine
    return write_cb(block_payload, block_payload_size, ctx);
  }

  // We have a region that needs to be read 32 bits at a time.
  //
  // Typically these are very small regions such as a memory mapped register address
  const uint32_t *word_data = block_payload;
  for (uint32_t i = 0; i < block_payload_size / 4; i++) {
    const uint32_t data = word_data[i];
    if (!write_cb(&data, sizeof(data), ctx)) {
      return false;
    }
  }

  return true;
}

bool memfault_coredump_write_block(eMfltCoredumpBlockType block_type,
                                   const void *block_payload, size_t block_payload_size,
                                   MfltCoredumpWriteCb write_cb, void *ctx) {
  const bool word_aligned_reads_only = false;
  return prv_write_block_with_address(block_type, block_payload, block_payload_size,
                                      0, write_cb, ctx, word_aligned_reads_only);
}

static eMfltCoredumpBlockType prv_region_type_to_storage_type(eMfltCoredumpRegionType type) {
  switch (type) {
    case kMfltCoredumpRegionType_ImageIdentifier:
    case kMfltCoredumpRegionType_Memory:
    case kMfltCoredumpRegionType_MemoryWordAccessOnly:
    default:
      return kMfltCoredumpBlockType_MemoryRegion;
  }
}

static eMfltCoredumpMachineType prv_get_machine_type(void) {
#if defined(MEMFAULT_UNITTEST)
  return kMfltCoredumpMachineType_None;
#else
#  if defined(__arm__) || defined(__ICCARM__)
  return kMfltCoredumpMachineType_ARM;
#  elif defined(__XTENSA__)
  return kMfltCoredumpMachineType_Xtensa;
#  else
#    error "Coredumps are not supported for target architecture"
#  endif
#endif
}

bool memfault_coredump_write_device_info_blocks(MfltCoredumpWriteCb write_cb, void *ctx) {
  struct MemfaultDeviceInfo info;
  memfault_platform_get_device_info(&info);

  if (info.device_serial) {
    if (!memfault_coredump_write_block(kMfltCoredumpRegionType_DeviceSerial,
                                       info.device_serial, strlen(info.device_serial),
                                       write_cb, ctx)) {
      return false;
    }
  }

  if (info.software_version) {
    if (!memfault_coredump_write_block(kMfltCoredumpRegionType_SoftwareVersion,
                                       info.software_version, strlen(info.software_version),
                                       write_cb, ctx)) {
      return false;
    }
  }

  if (info.software_type) {
    if (!memfault_coredump_write_block(kMfltCoredumpRegionType_SoftwareType,
                                       info.software_type, strlen(info.software_type),
                                       write_cb, ctx)) {
      return false;
    }
  }

  if (info.hardware_version) {
    if (!memfault_coredump_write_block(kMfltCoredumpRegionType_HardwareVersion,
                                       info.hardware_version, strlen(info.hardware_version),
                                       write_cb, ctx)) {
      return false;
    }
  }

  const sMfltMachineTypeBlock machine_block = {
    .machine_type = (uint32_t)prv_get_machine_type(),
  };
  return memfault_coredump_write_block(kMfltCoredumpRegionType_MachineType,
                                       &machine_block, sizeof(machine_block),
                                       write_cb, ctx);
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

static bool prv_write_trace_reason(MfltCoredumpWriteCb write_cb, uint32_t *offset,
                                   uint32_t trace_reason) {
  sMfltTraceReasonBlock trace_info = {
    .reason = trace_reason,
  };

  return memfault_coredump_write_block(kMfltCoredumpRegionType_TraceReason,
                                       &trace_info, sizeof(trace_info),
                                       write_cb, offset);
}

// When copying out some regions (for example, memory or register banks)
// we want to make sure we can do word-aligned accesses.
static void prv_insert_padding_if_necessary(MfltCoredumpWriteCb write_cb, uint32_t *offset) {
  #define MEMFAULT_WORD_SIZE 4
  const size_t remainder = *offset % MEMFAULT_WORD_SIZE;
  if (remainder == 0) {
    return;
  }

  #define MEMFAULT_MAX_PADDING_BYTES_NEEDED (MEMFAULT_WORD_SIZE - 1)
  uint8_t pad_bytes[MEMFAULT_MAX_PADDING_BYTES_NEEDED];

  size_t padding_needed = MEMFAULT_WORD_SIZE - remainder;
  memset(pad_bytes, 0x0, padding_needed);

  memfault_coredump_write_block(kMfltCoredumpRegionType_PaddingRegion,
      &pad_bytes, padding_needed, write_cb, offset);
}

//! Callback that will be called to write coredump data.
typedef bool(*MfltCoredumpReadCb)(uint32_t offset, void *data, size_t read_len);


static bool prv_get_info_and_header(sMfltCoredumpHeader *hdr_out,
                                    sMfltCoredumpStorageInfo *info_out,
                                    MfltCoredumpReadCb coredump_read_cb) {
  sMfltCoredumpStorageInfo info = { 0 };
  memfault_platform_coredump_storage_get_info(&info);
  if (info.size == 0) {
    return false; // no space for core files!
  }

  if (!coredump_read_cb(0, hdr_out, sizeof(*hdr_out))) {
    return false; // read failure, abort!
  }

  if (info_out) {
    *info_out = info;
  }
  return true;
}

static bool prv_coredump_get_header(sMfltCoredumpHeader *hdr_out,
                                    MfltCoredumpReadCb coredump_read_cb) {
  return prv_get_info_and_header(hdr_out, NULL, coredump_read_cb);
}

static bool prv_coredump_header_is_valid(const sMfltCoredumpHeader *hdr) {
  return (hdr && hdr->magic == MEMFAULT_COREDUMP_MAGIC);
}

static bool prv_write_regions(MfltCoredumpWriteCb write_cb, uint32_t *curr_offset,
                              const sMfltCoredumpRegion *regions, size_t num_regions) {
  for (size_t i = 0; i < num_regions; i++) {
    prv_insert_padding_if_necessary(write_cb, curr_offset);
    const sMfltCoredumpRegion *region = &regions[i];
    const bool word_aligned_reads_only =
        (region->type == kMfltCoredumpRegionType_MemoryWordAccessOnly);

    if (!prv_write_block_with_address(prv_region_type_to_storage_type(region->type),
                                      region->region_start, region->region_size,
                                      (uint32_t)(uintptr_t)region->region_start,
                                      write_cb, curr_offset, word_aligned_reads_only)) {
      return false;
    }
  }
  return true;
}

static bool prv_write_storage(const void *data, size_t len, void *ctx) {
  uint32_t *offset = ctx;
  if (!memfault_platform_coredump_storage_write(*offset, data, len)) {
    return false;
  }
  *offset += len;
  return true;
}

static bool prv_write_storage_compute_space_only(const void *data, size_t len, void *ctx) {
  // don't write any data but keep count of how many bytes would be written. This is used to
  // compute the total amount of space needed to store a coredump
  uint32_t *offset = ctx;
  *offset += len;
  return true;
}

static bool prv_write_coredump_sections(const sMemfaultCoredumpSaveInfo *save_info,
                                        bool compute_size_only, size_t *total_size) {
  sMfltCoredumpStorageInfo info = { 0 };
  sMfltCoredumpHeader hdr = { 0 };

  // are there some regions for us to save?
  size_t num_regions = save_info->num_regions;
  const sMfltCoredumpRegion *regions = save_info->regions;
  if ((regions == NULL) || (num_regions == 0)) {
    // sanity check that we got something valid from the caller
    return false;
  }

  if (!compute_size_only) {
    if (!memfault_platform_coredump_save_begin()) {
      return false;
    }

    // If we are saving a new coredump but one is already stored, don't overwrite it. This way an
    // the first issue which started the crash loop can be determined
    MfltCoredumpReadCb coredump_read_cb = memfault_platform_coredump_storage_read;
    if (!prv_get_info_and_header(&hdr, &info, coredump_read_cb)) {
      return false;
    }

    if (prv_coredump_header_is_valid(&hdr)) {
      return false; // don't overwrite what we got!
    }
  }

  // erase storage provided we aren't just computing the size
  if (!compute_size_only && !memfault_platform_coredump_storage_erase(0, info.size)) {
    return false;
  }

  const MfltCoredumpWriteCb storage_write_cb =
      compute_size_only ? prv_write_storage_compute_space_only : prv_write_storage;

  // We will write the header last as a way to mark validity
  uint32_t curr_offset = sizeof(hdr);

  const void *regs = save_info->regs;
  const size_t regs_size = save_info->regs_size;
  if (regs != NULL) {
    if (!memfault_coredump_write_block(kMfltCoredumpBlockType_CurrentRegisters,
        regs, regs_size, storage_write_cb, &curr_offset)) {
      return false;
    }
  }

  if (!memfault_coredump_write_device_info_blocks(storage_write_cb, &curr_offset)) {
    return false;
  }

  const uint32_t trace_reason = save_info->trace_reason;
  if (!prv_write_trace_reason(storage_write_cb, &curr_offset, trace_reason)) {
    return false;
  }

  // write out any architecture specific regions
  size_t num_arch_regions;
  const sMfltCoredumpRegion *arch_regions = memfault_coredump_get_arch_regions(&num_arch_regions);
  if (!prv_write_regions(storage_write_cb, &curr_offset, arch_regions, num_arch_regions)) {
    return false;
  }

  if (!prv_write_regions(storage_write_cb, &curr_offset, regions, num_regions)) {
    return false;
  }

  // we are done, mark things as valid
  size_t header_offset = 0;
  const bool success = memfault_coredump_write_header(curr_offset, storage_write_cb,
                                                      &header_offset);
  if (success) {
    *total_size = curr_offset;
  }
  return success;
}

MEMFAULT_WEAK
bool memfault_platform_coredump_save_begin(void) {
  return true;
}

size_t memfault_coredump_get_save_size(const sMemfaultCoredumpSaveInfo *save_info) {
  const bool compute_size_only = true;
  size_t total_size = 0;
  prv_write_coredump_sections(save_info, compute_size_only, &total_size);
  return total_size;
}

bool memfault_coredump_save(const sMemfaultCoredumpSaveInfo *save_info) {
  const bool compute_size_only = false;
  size_t total_size = 0;
  return prv_write_coredump_sections(save_info, compute_size_only, &total_size);
}

bool memfault_coredump_has_valid_coredump(size_t *total_size_out) {
  sMfltCoredumpHeader hdr = { 0 };
  // This routine is only called while the system is running so _always_ use the
  // memfault_coredump_read, which is safe to call while the system is running
  MfltCoredumpReadCb coredump_read_cb = memfault_coredump_read;
  if (!prv_coredump_get_header(&hdr, coredump_read_cb)) {
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

MEMFAULT_WEAK
bool memfault_coredump_read(uint32_t offset, void *buf, size_t buf_len) {
  return memfault_platform_coredump_storage_read(offset, buf, buf_len);
}

//! Expose a data source for use by the Memfault Packetizer
const sMemfaultDataSourceImpl g_memfault_coredump_data_source = {
  .has_more_msgs_cb = memfault_coredump_has_valid_coredump,
  .read_msg_cb = memfault_coredump_read,
  .mark_msg_read_cb = memfault_platform_coredump_storage_clear,
};
