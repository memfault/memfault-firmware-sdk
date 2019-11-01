#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Simple utilities for interacting with Base 128 Varints.
//!
//! More details about the encoding scheme can be found at:
//!   https://developers.google.com/protocol-buffers/docs/encoding#varints

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The maximum length of a VARINT encoding for a uint32_t in bytes
#define MEMFAULT_UINT32_MAX_VARINT_LENGTH 5

//! Given a uint32_t, encodes it as a Varint
//!
//! @param[in] value The value to encode
//! @param[out] buf The buffer to write the Varint encoding too. Needs to be appropriately sized to
//!   hold the encoding. The maximum encoding length is MEMFAULT_UINT32_MAX_VARINT_LENGTH
//! @return The number of bytes written into the buffer
size_t memfault_encode_varint_u32(uint32_t value, void *buf);

#ifdef __cplusplus
}
#endif
