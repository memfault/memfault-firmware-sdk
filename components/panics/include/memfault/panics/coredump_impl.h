#pragma once

//! @file

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

//! Coredump block types
typedef enum MfltCoredumpBlockType  {
  kMfltCoredumpBlockType_CurrentRegisters = 0,
  kMfltCoredumpBlockType_MemoryRegion = 1,
  kMfltCoredumpRegionType_DeviceSerial = 2,
  kMfltCoredumpRegionType_FirmwareVersion = 3,
  kMfltCoredumpRegionType_HardwareRevision = 4,
  kMfltCoredumpRegionType_TraceReason = 5,
  kMfltCoredumpRegionType_PaddingRegion = 6,
  kMfltCoredumpRegionType_MachineType = 7,
  kMfltCoredumpRegionType_VendorCoredumpEspIdfV2ToV3_1 = 8,
} eMfltCoredumpBlockType;

//! Callback that will be called to write coredump data.
//! @param data Data to be written
//! @param data_len Length of data in bytes
//! @param ctx User data pointer as passed into the memfault_coredump_write_...() function
typedef bool(*MfltCoredumpWriteCb)(const void *data, size_t data_len, void *ctx);

//! Writes a block of given type and size and optionally also writes the payload as well.
//! @param block_type The type of block to write
//! @param block_payload The payload that should be written after the block header. NULL can be used to skip writing the
//!        payload (this works regardless of the value of block_payload_size).
//! @param block_payload_size The size of the block_payload in bytes.
//! @param write_cb The function that will be called to write out the block header and (optionally) the block payload.
//! @param ctx User data pointer that will be passed to write_cb.
bool memfault_coredump_write_block(eMfltCoredumpBlockType block_type,
    const void *block_payload, size_t block_payload_size,
    MfltCoredumpWriteCb write_cb, void *ctx);

//! Writes the coredump header.
//! @param total_coredump_size The total size of the entire coredump (including the header) data in bytes.
//! @param write_cb The function that will be called to write out the block header and (optionally) the block payload.
//! @param ctx User data pointer that will be passed to write_cb.
bool memfault_coredump_write_header(size_t total_coredump_size,
    MfltCoredumpWriteCb write_cb, void *ctx);

//! Attempts to write the device serial, firmware version, hardware revision and machine architecture blocks. When
//! information is missing, the block in question is skipped.
//! @param write_cb The function that will be called to write out the block header and (optionally) the block payload.
//! @param ctx User data pointer that will be passed to write_cb.
void memfault_coredump_write_device_info_blocks(MfltCoredumpWriteCb write_cb, void *ctx);
