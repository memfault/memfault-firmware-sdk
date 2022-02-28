//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/util/base64.h"

#include <stddef.h>
#include <stdint.h>

static char prv_get_char_from_word(uint32_t word, int offset) {
  const char *base64_table =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint8_t base64_mask = 0x3f; // one char per 6 bits
  return base64_table[(word >> (offset * 6)) & base64_mask];
}

void memfault_base64_encode(const void *buf, size_t buf_len, void *base64_out) {
  const uint8_t *bin_inp = (const uint8_t *)buf;
  char *out_bufp = (char *)base64_out;

  int curr_idx = 0;

  for (size_t bin_idx = 0; bin_idx < buf_len;  bin_idx += 3) {
    const uint32_t byte0 = bin_inp[bin_idx];
    const uint32_t byte1 = ((bin_idx + 1) < buf_len) ? bin_inp[bin_idx + 1] : 0;
    const uint32_t byte2 = ((bin_idx + 2) < buf_len) ? bin_inp[bin_idx + 2] : 0;
    const uint32_t triple = (byte0 << 16) + (byte1 << 8) + byte2;

    out_bufp[curr_idx++] = prv_get_char_from_word(triple, 3);
    out_bufp[curr_idx++] = prv_get_char_from_word(triple, 2);
    out_bufp[curr_idx++] = ((bin_idx + 1) < buf_len) ? prv_get_char_from_word(triple, 1) : '=';
    out_bufp[curr_idx++] = ((bin_idx + 2) < buf_len) ? prv_get_char_from_word(triple, 0) : '=';
  }
}

void memfault_base64_encode_inplace(void *buf, size_t bin_len) {
  // When encoding with base64, every 3 bytes is represented as 4 characters. If the input binary
  // blob is not a multiple of 3, we will need to use the "=" padding character. Here we determine
  // what index to start encoding at that will end on a multiple of 3 boundary
  const size_t remainder = bin_len % 3;
  const size_t start_idx = (remainder == 0) ? bin_len - 3 : bin_len - remainder;

  // The index to write the base64 character conversion into starting with the last location
  const size_t encoded_len = MEMFAULT_BASE64_ENCODE_LEN(bin_len);
  int curr_idx = (int)encoded_len - 1;

  const uint8_t *bin_inp = (const uint8_t *)buf;
  char *out_bufp = (char *)buf;

  // NB: By encoding from last set of 3 bytes to first set, we can edit the buffer inplace
  // without clobbering the input data we need to determine the encoding
  for (int bin_idx = (int)start_idx; bin_idx >= 0; bin_idx -= 3) {
    const uint32_t byte0 = bin_inp[bin_idx];
    const uint32_t byte1 = ((bin_idx + 1) < (int)bin_len) ? bin_inp[bin_idx + 1] : 0;
    const uint32_t byte2 = ((bin_idx + 2) < (int)bin_len) ? bin_inp[bin_idx + 2] : 0;
    const uint32_t triple = (byte0 << 16) + (byte1 << 8) + byte2;

    out_bufp[curr_idx--] = ((bin_idx + 2) < (int)bin_len) ? prv_get_char_from_word(triple, 0) : '=';
    out_bufp[curr_idx--] = ((bin_idx + 1) < (int)bin_len) ? prv_get_char_from_word(triple, 1) : '=';
    out_bufp[curr_idx--] = prv_get_char_from_word(triple, 2);
    out_bufp[curr_idx--] = prv_get_char_from_word(triple, 3);
  }
}
