#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Many embedded projects have custom data formats which can be useful to analyze when debugging
//! issue (i.e trace data, proprietary coredumps, logs). Memfault allows this arbitrary information
//! to be bundled as a "Custom Data Recording" (CDR). The following API provides a flexible way
//! for this type of data to be published to the Memfault cloud. Within the Memfault cloud,
//! recorded data can then easily be looked up by reason, time, and device.
//!
//! To start reporting custom data a user of the SDK must do the following:
//!
//!  1. Implement the sMemfaultCdrSourceImpl API for the data to be captured:
//!     static sMemfaultCdrSourceImpl s_my_custom_data_recording_source = {
//!         [... fill me in ...]
//!     }
//!  2. Register the implementation with this module by calling:
//!     memfault_cdr_register_source(&s_my_custom_data_recording_source)
//!
//! See documentation below for more tips on how to implement the API.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "memfault/core/platform/system_time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_CDR_BINARY  "application/octet-stream"
#define MEMFAULT_CDR_TEXT "text/plain"
#define MEMFAULT_CDR_CSV "text/csv"

typedef struct MemfaultCdrMetadata {
  //! The time the recording was started:
  //!  .start_time = (sMemfaultCurrentTime) {
  //!     .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
  //!     .info = { .unix_timestamp_secs = ... },
  //!  }
  //!
  //! If no start_time is known, simply set the type to kMemfaultCurrentTimeType_Unknown
  //! and Memfault will use the time the data arrives on the server - duration_ms to approximate
  //! the start.
  sMemfaultCurrentTime start_time;

  //! The format of the CDR. Memfault uses this as a hint for visualization in the UI
  //!
  //! Typically this will be one of the MEMFAULT_CDR_* defines from above:
  //!
  //! const char *mimetypes = { MEMFAULT_CDR_BINARY };
  //! MemfaultCdrMetadata my_cdr = (MemfaultCdrMetadata) {
  //!   [...]
  //!   .mimetypes = mimetypes,
  //!   .num_mimetypes = MEMFAULT_ARRAY_SIZE(mimetypes),
  //! }
  //!
  //! @note List is expected to be ordered from most specific to most generic description when
  //! multiple types are listed.
  const char **mimetypes;
  size_t num_mimetypes;

  //! The total size of the CDR data (in bytes)
  uint32_t data_size_bytes;

  //! The duration of time the recording tracks (can be 0 if unknown).
  uint32_t duration_ms; // optional

  //! The reason the data was captured (i.e "ble connection failure", "wifi stack crash")
  //!
  //! @note A project can have at most 100 unique reasons and the length of a given reason can not
  //! exceed 100 characters.
  const char *collection_reason;
} sMemfaultCdrMetadata;

typedef struct MemfaultCdrSourceImpl {
  //! Called to determine if a CDR is available
  //!
  //! @param metadata Metadata associated with recording. See comments within
  //! sMemfaultCdrMetadata typedef for more info.
  //!
  //! @note If a recording is available, metadata must be populated.
  //!
  //! @return true if a recording is available, false otherwise
  bool (*has_cdr_cb)(sMemfaultCdrMetadata *metadata);

  //! Called to read the data within a CDR.
  //!
  //! @note It is expected the data being returned here is from the CDR
  //! returned in the last "has_cdr_cb" call and that it is
  //! safe to call this function multiple times
  //!
  //! @param offset The offset within the CDR data to read
  //! @param data The buffer to populate with CDR data
  //! @param data_len The size of data to fill in the buffer
  //!
  //! return true if read was succesful and buf was entirely filled, false otherwise
  bool (*read_data_cb)(uint32_t offset, void *data, size_t data_len);

  //! Called once the recording has been processed
  //!
  //! At this point, the recording should be cleared and a subsequent call to has_cdr_cb
  //! should either return metadata for a new recording or false
  void (*mark_cdr_read_cb)(void);
} sMemfaultCdrSourceImpl;

//! Register a Custom Data Recording generator to publish data to the Memfault cloud
//!
//! @param impl The CDR source to register
//!
//! @note We recommend invoking this function at bootup and expect that the "impl" passed
//! is of static or global scope
//!
//! @return true if registration was succesful or false if the storage was full. If false is returned
//! MEMFAULT_CDR_MAX_DATA_SOURCES needs to be increased.
bool memfault_cdr_register_source(const sMemfaultCdrSourceImpl *impl);

#ifdef __cplusplus
}
#endif
