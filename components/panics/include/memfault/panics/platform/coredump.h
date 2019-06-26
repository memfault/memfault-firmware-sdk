#pragma once

//! @file
//!
//! @brief
//! Dependency functions required in order to use the Memfault coredump API
//!
//! @note The expectation is that all these functions are safe to call from ISRs and with
//!   interrupts disabled.
//! @note It's also expected the caller has implemented memfault_platform_get_device_info(). That way
//!   we can save off information that allows for the coredump to be further decoded server side

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum MfltCoredumpRegionType {
  kMfltCoredumpRegionType_Memory,
  kMfltCoredumpRegionType_ImageIdentifier,
} eMfltCoredumpRegionType;

//! Convenience macro to define a sMfltCoredumpRegion of type kMfltCoredumpRegionType_Memory.
#define MEMFAULT_COREDUMP_MEMORY_REGION_INIT(_start, _size) \
  (sMfltCoredumpRegion) { \
    .type = kMfltCoredumpRegionType_Memory, \
    .region_start = _start, \
    .region_size = _size, \
  }

typedef struct MfltCoredumpRegion {
  eMfltCoredumpRegionType type;
  const void *region_start;
  uint32_t region_size;
} sMfltCoredumpRegion;

//! Returns an array of the regions to capture when the system crashes
//! @param num_regions The number of regions in the list returned
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(size_t *num_regions);

typedef struct MfltCoredumpStorageInfo {
  //! The size of the coredump storage region (must be greater than the space needed to capture all
  //! the regions returned from @ref memfault_platform_coredump_get_regions)
  size_t size;
  //! Sector size for storage medium used for coredump storage
  size_t sector_size;
} sMfltCoredumpStorageInfo;

//! Return info pertaining to the region a coredump will be stored in
void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info);

//! Issue write to the platforms coredump storage region
//!
//! @param offset the offset within the platform coredump storage region to write to
//! @param data opaque data to write
//! @param data_len length of data to write in bytes
bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                                     size_t data_len);

//! Read from platforms coredump storage region
//!
//! @param offset the offset within the platform coredump storage region to read from
//! @param data the buffer to read the data into
//! @param read_len length of data to read in bytes
bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                                    size_t read_len);

//! Erase a region of the platforms coredump storage
//!
//! @param offset to start erasing within the coredump storage region (must be sector_size
//!   aligned)
//! @param erase_size The amount of bytes to erase (must be in sector_size units)
bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size);

//! Invalidate any saved coredumps within the platform storage coredump region
//!
//! Coredumps are persisted until this function is called so this is useful to call after a
//! coredump has been read so a new coredump will be captured upon next crash
//!
//! @note a coredump region can be invalidated by zero'ing out or erasing the first sector
//! being used for storage
void memfault_platform_coredump_storage_clear(void);
