#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! A backend which serializes logs that have been compressed using
//! "memfault/core/compact_log_helpers.h" into an array of CBOR encoded arguments.
//! This enables logs to be easily inserted into Memfault events or base64/hexdump'd out
//! over the CLI.

#include <stdbool.h>

#include "memfault/util/cbor.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Serializes a "compact" log into a CBOR array representation
//!
//! @param encoder The cbor encoder to write into
//! @param log_id A unique identifier for the log being serialized
//! @param compressed_fmt The formatters for the log. See MFLT_GET_COMPRESSED_LOG_FMT in
//!  "memfault/core/compact_log_helpers.h" for more context
//! @param args The list of arguments to encode
//!
//! @return true if serialization succeeded, false otherwise
bool memfault_vlog_compact_serialize(sMemfaultCborEncoder *encoder, uint32_t log_id,
                                     uint32_t compressed_fmt, va_list args);

//! Serializes a fallback entry, when the compact log is too large to fit the
//! provided log entry limit.
//!
//! @param encoder The cbor encoder to write into
//! @param log_id A unique identifier for the log being serialized
//! @param serialized_len The computed serialized length of the original compact log
//!
//! @return true if serialization succeeded, false otherwise
bool memfault_vlog_compact_serialize_fallback_entry(sMemfaultCborEncoder *encoder, uint32_t log_id,
                                                    uint32_t serialized_len);

//! Same as "memfault_vlog_compact_serialize" but takes a variable list of arguments
bool memfault_log_compact_serialize(sMemfaultCborEncoder *encoder, uint32_t log_id,
                                    uint32_t compressed_fmt, ...);

#ifdef __cplusplus
}
#endif
