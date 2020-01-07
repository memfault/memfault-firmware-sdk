//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <stddef.h>
  #include <stdint.h>
  #include <stdio.h>
  #include <string.h>

  #include "memfault/core/math.h"
  #include "memfault/util/chunk_transport.h"

  static const uint8_t s_test_msg[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa };
  static const uint8_t *s_active_msg = NULL;

  static void prv_chunk_msg(uint32_t offset, void *out_buf, size_t out_buf_len) {
    CHECK(s_active_msg != NULL);
    memcpy(out_buf, &s_active_msg[offset], out_buf_len);
  }
}

TEST_GROUP(MemfaultChunkTransport){
  void setup() {
    s_active_msg = &s_test_msg[0];
  }
  void teardown() {
  }
};

static void prv_check_chunk(sMfltChunkTransportCtx *ctx, bool expect_md, size_t receive_buf_len,
                            const void *expected_chunk, size_t expected_chunk_len) {
  uint8_t actual_chunk[receive_buf_len];
  memset(actual_chunk, 0x0, receive_buf_len);
  bool md = memfault_chunk_transport_get_next_chunk(ctx, &actual_chunk[0], &receive_buf_len);
  LONGS_EQUAL(ctx->total_size + 1 + 2, ctx->single_chunk_message_length);


  LONGS_EQUAL(expect_md, md);
  LONGS_EQUAL(expected_chunk_len, receive_buf_len);
  MEMCMP_EQUAL(expected_chunk, actual_chunk, expected_chunk_len);
}


TEST(MemfaultChunkTransport, Test_ChunkerMultiPart) {
  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_test_msg),
    .read_msg = &prv_chunk_msg,
  };

  const size_t receive_buf_size = 9;
  const bool md = true;

  const uint8_t expected_msg_1[] = { 0x40, 0x09, 0x1b, 0x13, 0x1, 0x2, 0x3, 0x4, 0x5 };
  prv_check_chunk(&ctx, md, receive_buf_size, &expected_msg_1, sizeof(expected_msg_1));

  const uint8_t expected_msg_2[] = { 0x80, 0x5, 0x6, 0x7, 0x8, 0xa };
  prv_check_chunk(&ctx, !md, receive_buf_size, &expected_msg_2, sizeof(expected_msg_2));
}

TEST(MemfaultChunkTransport, Test_BufTooSmall) {
  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_test_msg),
    .read_msg = &prv_chunk_msg,
  };

  size_t receive_buf_len = 8;
  uint8_t receive_buf[receive_buf_len];
  bool md = memfault_chunk_transport_get_next_chunk(&ctx, &receive_buf[0], &receive_buf_len);
  CHECK(md);
  LONGS_EQUAL(0, receive_buf_len);
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsg) {
  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_test_msg),
    .read_msg = &prv_chunk_msg,
  };

  const bool md = true;

  const uint8_t expected_msg_all[] = { 0x00, 0x1b, 0x13, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa };
  prv_check_chunk(&ctx, !md, sizeof(expected_msg_all), &expected_msg_all, sizeof(expected_msg_all));
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsgOversizeBuffer) {
  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_test_msg),
    .read_msg = &prv_chunk_msg,
  };

  const uint8_t expected_msg_all[] = { 0x00, 0x1b, 0x13, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa };
  const size_t expected_chunk_len = sizeof(expected_msg_all);
  const size_t known_pattern_len = 10;
  uint8_t actual_chunk[expected_chunk_len + known_pattern_len];
  memset(actual_chunk, 0x0, sizeof(actual_chunk));

  size_t buf_len = sizeof(actual_chunk);
  const bool md = memfault_chunk_transport_get_next_chunk(&ctx, &actual_chunk[0], &buf_len);
  CHECK(!md);
  LONGS_EQUAL(ctx.total_size + 1 + 2, ctx.single_chunk_message_length);
  LONGS_EQUAL(expected_chunk_len, buf_len);
  MEMCMP_EQUAL(expected_msg_all, actual_chunk, expected_chunk_len);

  // the remainder of the buffer should have been scrubbed with a known pattern
  uint8_t pattern[known_pattern_len];
  memset(pattern, 0xBA, sizeof(pattern));
  MEMCMP_EQUAL(&actual_chunk[expected_chunk_len], pattern, sizeof(pattern));
}


TEST(MemfaultChunkTransport, Test_ChunkerBigMessage) {
  uint8_t s_big_msg[4072] = { 0x1, 0x2, 0x3 };
  s_active_msg = &s_big_msg[0];

  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_big_msg),
    .read_msg = &prv_chunk_msg,
  };

  // get info before sending the message
  memfault_chunk_transport_get_chunk_info(&ctx);
  LONGS_EQUAL(ctx.total_size + 1 + 2, ctx.single_chunk_message_length);
  LONGS_EQUAL(0, ctx.read_offset);

  // Let's just make sure the sequencer does okay when it takes more than 1 byte to encode a varint
  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x40, 0xe8, 0x1f, 0xF4, 0x79, 0x1, 0x2, 0x3, 0x0};
  prv_check_chunk(&ctx, md, sizeof(expected_msg_1), &expected_msg_1, sizeof(expected_msg_1));

  // read 4000 bytes (& 1 byte hdr & 1 byte varint)
  uint8_t read_buffer[4069];
  size_t read_buffer_size = sizeof(read_buffer);
  bool md_avail = memfault_chunk_transport_get_next_chunk(&ctx, &read_buffer[0], &read_buffer_size);
  LONGS_EQUAL(read_buffer[0], 0xC0); // should be a continuation header
  CHECK(md_avail);
  LONGS_EQUAL(sizeof(read_buffer), read_buffer_size);

  // size info should not change if we are in the middle of sending a message
  memfault_chunk_transport_get_chunk_info(&ctx);
  LONGS_EQUAL(ctx.total_size + 1 + 2, ctx.single_chunk_message_length);
  LONGS_EQUAL(4071, ctx.read_offset);

  const uint8_t expected_msg_2[] = { 0x80, 0xe7, 0x1f, 0x0};
  prv_check_chunk(&ctx, !md, 20 /* oversize buffer */, &expected_msg_2, sizeof(expected_msg_2));
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsgMultiChunkEnabled) {
  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_test_msg),
    .read_msg = &prv_chunk_msg,
    .enable_multi_call_chunk = true,
  };

  const bool md = true;

  const uint8_t expected_msg_all[] = { 0x00, 0x1b, 0x13, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa };
  prv_check_chunk(&ctx, !md, sizeof(expected_msg_all), &expected_msg_all, sizeof(expected_msg_all));
}

TEST(MemfaultChunkTransport, Test_ChunkerMultiCallSingleChunk) {
  uint8_t s_big_msg[4072] = { 0x1, 0x2, 0x3 };
  s_active_msg = &s_big_msg[0];

  sMfltChunkTransportCtx ctx = {
    .total_size = MEMFAULT_ARRAY_SIZE(s_big_msg),
    .read_msg = &prv_chunk_msg,
    .enable_multi_call_chunk = true,
  };

  // Let's just make sure the sequencer does okay when it takes more than 1 byte to encode a varint
  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x00, 0xF4, 0x79, 0x1, 0x2, 0x3, 0x0, 0x0, 0x0};
  prv_check_chunk(&ctx, md, sizeof(expected_msg_1), &expected_msg_1, sizeof(expected_msg_1));

  // read all bytes except for the last one
  uint8_t read_buffer[4065];
  size_t read_buffer_size = sizeof(read_buffer);
  bool md_avail = memfault_chunk_transport_get_next_chunk(&ctx, &read_buffer[0], &read_buffer_size);
  CHECK(md_avail);
  LONGS_EQUAL(sizeof(read_buffer), read_buffer_size);

  const uint8_t expected_msg_2[] = { 0x0 };
  prv_check_chunk(&ctx, !md, 20 /* oversize buffer */, &expected_msg_2, sizeof(expected_msg_2));
}
