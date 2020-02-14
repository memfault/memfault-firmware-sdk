//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "memfault/util/rle.h"
#include "memfault/core/math.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>
}

TEST_GROUP(MemfaultRle){
  void setup() {
  }
  void teardown() {
  }
};

typedef struct {
  const uint8_t *orig_buf;
  size_t orig_buf_len;
  uint8_t *write_buf;
  size_t write_buf_len;
  size_t write_buf_offset;
} sMemfaultRleResultCtx;

static void prv_update_result_buf(sMemfaultRleResultCtx *result_ctx,
                                  const sMemfaultRleWriteInfo *write_info) {
  if (!write_info->available) {
    return;
  }
  const size_t idx = result_ctx->write_buf_offset;
  const size_t write_len = write_info->write_len;
  const size_t end_offset = idx + write_len + write_info->header_len;
  CHECK(end_offset <= result_ctx->write_buf_len);

  uint8_t *write_buf = &result_ctx->write_buf[idx];
  const uint8_t *read_buf = &result_ctx->orig_buf[write_info->write_start_offset];

  memcpy(&write_buf[0], write_info->header, write_info->header_len);
  memcpy(&write_buf[write_info->header_len], read_buf, write_len);
  result_ctx->write_buf_offset += write_info->header_len + write_len;
}

static void prv_encode_with_fill_interval(
    sMemfaultRleResultCtx *result_ctx, size_t total_size, size_t fill_call_size) {
  sMemfaultRleCtx ctx = { 0 };

  memfault_rle_encode(&ctx, NULL, 0); // should be a no-op
  for (size_t i = 0; i < result_ctx->orig_buf_len;) {
    const size_t read_len = MEMFAULT_MIN(fill_call_size, result_ctx->orig_buf_len - i);
    const size_t bytes_encoded = memfault_rle_encode(&ctx, &result_ctx->orig_buf[i], read_len);
    prv_update_result_buf(result_ctx, &ctx.write_info);
    i += bytes_encoded;
  }
  memfault_rle_encode_finalize(&ctx);
  prv_update_result_buf(result_ctx, &ctx.write_info);
  LONGS_EQUAL(total_size, ctx.total_rle_size);
}

static void prv_encode(sMemfaultRleResultCtx *result_ctx, size_t total_size) {
  prv_encode_with_fill_interval(result_ctx, total_size, total_size);
}

static void prv_check_pattern(const uint8_t *in, size_t in_len,
                              const uint8_t *expected_out, size_t expected_out_len) {
  uint8_t encode_buf[expected_out_len];
  memset(encode_buf, 0x0, sizeof(encode_buf));

  sMemfaultRleResultCtx result_ctx = { 0 };
  result_ctx.orig_buf = in;
  result_ctx.orig_buf_len = in_len;
  result_ctx.write_buf = encode_buf;
  result_ctx.write_buf_len = sizeof(encode_buf);
  prv_encode(&result_ctx, expected_out_len);
  MEMCMP_EQUAL(expected_out, encode_buf, expected_out_len);
}

TEST(MemfaultRle, Test_QuickTransitions) {
  const uint8_t pattern[] = {
    0x1,
    0x2, 0x2,
    0x3,
    0x4, 0x4,
    0x5,
    0x6, 0x6,
    0x7,
    0x8, 0x8
  };

  const uint8_t expected[] = { 0x17, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04,
                               0x05, 0x06, 0x06, 0x07, 0x08, 0x08, };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_2and3RunSequences) {
  const uint8_t pattern[] = {
    0x1,
    0x2, 0x2, // 2-byte pattern too short, gets included in current non-repeating sequence
    0x3,
    0x4, 0x4, 0x4, // 3-byte pattern should start new repeating sequence
    0x5,
    0x6, 0x6, 0x6,
    0x7, 0x7,
    0x8, 0x8
  };

  const uint8_t expected[] = { 0x07, 0x01, 0x02, 0x02, 0x03, 0x06, 0x04, 0x01,
                               0x05, 0x06, 0x06, 0x04, 0x07, 0x04, 0x08 };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_SingleByte) {
  const uint8_t pattern[] = {
    0x1,
  };

  const uint8_t expected[] = { 0x01, 0x01 };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_LongRepeat) {
  uint8_t pattern[4096];
  memset(pattern, 0xEF, sizeof(pattern));

  const uint8_t expected[] = { 0x80, 0x40, 0xEF };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_EndWithRepeatSequence) {
  const uint8_t pattern[] = { 0x1, 0x2, 0x3, 0x3, 0x3, 0x3 };

  const uint8_t expected[] = { 0x03, 0x01, 0x02, 0x08, 0x03 };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_EndWithNonRepeat) {
  const uint8_t pattern[] = { 0x1, 0x2, 0x3, 0x3, 0x3, 0x3, 0x5, 0x6, 0x7 };

  const uint8_t expected[] = { 0x03, 0x01, 0x02, 0x08, 0x03, 0x05, 0x05, 0x06, 0x07  };
  prv_check_pattern(pattern, sizeof(pattern), expected, sizeof(expected));
}

TEST(MemfaultRle, Test_VariousEncoderSizes) {
  uint8_t pattern[400] = { 0x1, 0x2, 0x3, 0x3, 0x3, 0x3, 0x5, 0x6, 0x7, };

  const size_t expected_total_size = 12;

  for (size_t i = 1; i < sizeof(pattern); i++) {
    uint8_t encode_buf[expected_total_size];
    memset(encode_buf, 0x0, sizeof(encode_buf));

    sMemfaultRleResultCtx result_ctx = { 0 };
    result_ctx.orig_buf = &pattern[0];
    result_ctx.orig_buf_len = sizeof(pattern);
    result_ctx.write_buf = encode_buf;
    result_ctx.write_buf_len = sizeof(encode_buf);

    prv_encode_with_fill_interval(&result_ctx, expected_total_size, i);
    const uint8_t expected[] =
        { 0x03, 0x01, 0x02, 0x08, 0x03, 0x05, 0x05, 0x06, 0x07, 0x8E, 0x06, 0x00 };
    MEMCMP_EQUAL(expected, encode_buf, expected_total_size);
  }
}
