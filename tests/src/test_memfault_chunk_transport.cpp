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
  static sMfltChunkTransportCtx s_chunk_ctx;

  typedef struct {
    size_t total_bytes_read;
    size_t last_offset;
  } sMfltChunkReadStats;

  static sMfltChunkReadStats s_chunk_read_stats;

  static void prv_chunk_msg(uint32_t offset, void *out_buf, size_t out_buf_len) {
    // One property enforced on the chunker is that all the reads are performed sequentially. This
    // way the offsets which will be accessed prior to invoking the chunker are known.
    CHECK(offset >= s_chunk_read_stats.last_offset);
    s_chunk_read_stats.last_offset = offset;
    s_chunk_read_stats.total_bytes_read += out_buf_len;
    CHECK(s_active_msg != NULL);
    memcpy(out_buf, &s_active_msg[offset], out_buf_len);
  }
}

TEST_GROUP(MemfaultChunkTransport){
  void setup() {
    s_active_msg = &s_test_msg[0];
    memset(&s_chunk_read_stats, 0x0, sizeof(s_chunk_read_stats));
    memset(&s_chunk_ctx, 0x0, sizeof(s_chunk_ctx));

    // restore defaults
    s_chunk_ctx.total_size = MEMFAULT_ARRAY_SIZE(s_test_msg);
    s_chunk_ctx.read_msg = &prv_chunk_msg;
  }
  void teardown() {
    if (s_chunk_read_stats.total_bytes_read == 0) {
      return; // a failure test case where no data got read
    }

    // Another property we impose on the chunker is that it should only need to read the message
    // being sent once. This is a performance optimization for situations where the backing storage
    // medium (i.e flash) can be slow to access.
    LONGS_EQUAL(s_chunk_ctx.total_size, s_chunk_read_stats.total_bytes_read);
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
  const size_t receive_buf_size = 9;
  const bool md = true;

  const uint8_t expected_msg_1[] = { 0x48, 0x09, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };
  prv_check_chunk(&s_chunk_ctx, md, receive_buf_size, &expected_msg_1, sizeof(expected_msg_1));

  const uint8_t expected_msg_2[] = { 0x80, 0x7, 0x8, 0xa, 0x1b, 0x13 };
  prv_check_chunk(&s_chunk_ctx, !md, receive_buf_size, &expected_msg_2, sizeof(expected_msg_2));
}

TEST(MemfaultChunkTransport, Test_ChunkerMultiPartLastMessageJustCrc) {
  static const uint8_t test_msg_long[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                                           0x8, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
  s_chunk_ctx.total_size = sizeof(test_msg_long);
  s_active_msg = &test_msg_long[0];

  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x48, 0x0E, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };
  size_t receive_buf_size = sizeof(expected_msg_1);
  prv_check_chunk(&s_chunk_ctx, md, receive_buf_size, &expected_msg_1, sizeof(expected_msg_1));

  const uint8_t expected_msg_2[] = { 0xC0, 0x07, 0x8, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
  // The CRC is two bytes so make sure if there is only 1 extra byte, the crc is not written and
  // one more chunk needs to be sent
  receive_buf_size = sizeof(expected_msg_2) + 1;
  prv_check_chunk(&s_chunk_ctx, md, receive_buf_size, &expected_msg_2, sizeof(expected_msg_2));

  const uint8_t expected_msg_3[] = { 0x80, 0xE, 0x4c, 0xdc };
  receive_buf_size = MEMFAULT_MIN_CHUNK_BUF_LEN;
  prv_check_chunk(&s_chunk_ctx, !md, receive_buf_size, &expected_msg_3, sizeof(expected_msg_3));
}


TEST(MemfaultChunkTransport, Test_ChunkerMultiCallChunkLastMessageJustCrc) {
  static const uint8_t test_msg_long[] = {
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8,
    0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };
  s_chunk_ctx.total_size = sizeof(test_msg_long);
  s_active_msg = &test_msg_long[0];
  s_chunk_ctx.enable_multi_call_chunk = true;

  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x08, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8 };
  size_t receive_buf_size = sizeof(expected_msg_1);
  prv_check_chunk(&s_chunk_ctx, md, receive_buf_size, &expected_msg_1, sizeof(expected_msg_1));

  const uint8_t expected_msg_2[] = { 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };
  // The CRC is two bytes so make sure if there is only 1 extra byte, the crc is not written and
  // one more chunk needs to be sent
  receive_buf_size = sizeof(expected_msg_2) + 1;
  prv_check_chunk(&s_chunk_ctx, md, receive_buf_size, &expected_msg_2, sizeof(expected_msg_2));

  const uint8_t expected_msg_3[] = { 0x13, 0xdb };
  receive_buf_size = MEMFAULT_MIN_CHUNK_BUF_LEN;
  prv_check_chunk(&s_chunk_ctx, !md, receive_buf_size, &expected_msg_3, sizeof(expected_msg_3));
}

TEST(MemfaultChunkTransport, Test_BufTooSmall) {
  size_t receive_buf_len = 8;
  uint8_t receive_buf[receive_buf_len];
  bool md = memfault_chunk_transport_get_next_chunk(&s_chunk_ctx, &receive_buf[0], &receive_buf_len);
  CHECK(md);
  LONGS_EQUAL(0, receive_buf_len);
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsg) {
  const bool md = true;

  const uint8_t expected_msg_all[] = { 0x08, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa, 0x1b, 0x13 };
  prv_check_chunk(&s_chunk_ctx, !md, sizeof(expected_msg_all), &expected_msg_all, sizeof(expected_msg_all));
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsgOversizeBuffer) {
  const uint8_t expected_msg_all[] = { 0x08, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa, 0x1b, 0x13 };
  const size_t expected_chunk_len = sizeof(expected_msg_all);
  const size_t known_pattern_len = 10;
  uint8_t actual_chunk[expected_chunk_len + known_pattern_len];
  memset(actual_chunk, 0x0, sizeof(actual_chunk));

  size_t buf_len = sizeof(actual_chunk);
  const bool md = memfault_chunk_transport_get_next_chunk(&s_chunk_ctx, &actual_chunk[0], &buf_len);
  CHECK(!md);
  LONGS_EQUAL(s_chunk_ctx.total_size + 1 + 2, s_chunk_ctx.single_chunk_message_length);
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

  s_chunk_ctx.total_size = MEMFAULT_ARRAY_SIZE(s_big_msg);

  // get info before sending the message
  memfault_chunk_transport_get_chunk_info(&s_chunk_ctx);
  LONGS_EQUAL(s_chunk_ctx.total_size + 1 + 2, s_chunk_ctx.single_chunk_message_length);
  LONGS_EQUAL(0, s_chunk_ctx.read_offset);

  // Let's just make sure the sequencer does okay when it takes more than 1 byte to encode a varint
  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x48, 0xe8, 0x1f, 0x1, 0x2, 0x3, 0x0, 0x0, 0x0};
  prv_check_chunk(&s_chunk_ctx, md, sizeof(expected_msg_1), &expected_msg_1, sizeof(expected_msg_1));

  // read 4000 bytes (& 1 byte hdr & 1 byte varint)
  uint8_t read_buffer[4067];
  size_t read_buffer_size = sizeof(read_buffer);
  bool md_avail = memfault_chunk_transport_get_next_chunk(&s_chunk_ctx, &read_buffer[0], &read_buffer_size);
  LONGS_EQUAL(read_buffer[0], 0xC0); // should be a continuation header
  CHECK(md_avail);
  LONGS_EQUAL(sizeof(read_buffer), read_buffer_size);

  // size info should not change if we are in the middle of sending a message
  memfault_chunk_transport_get_chunk_info(&s_chunk_ctx);
  LONGS_EQUAL(s_chunk_ctx.total_size + 1 + 2, s_chunk_ctx.single_chunk_message_length);
  LONGS_EQUAL(4071, s_chunk_ctx.read_offset);

  const uint8_t expected_msg_2[] = { 0x80, 0xe7, 0x1f, 0x0, 0xF4, 0x79};
  prv_check_chunk(&s_chunk_ctx, !md, 20 /* oversize buffer */, &expected_msg_2, sizeof(expected_msg_2));
}

TEST(MemfaultChunkTransport, Test_ChunkerSingleMsgMultiChunkEnabled) {
  s_chunk_ctx.enable_multi_call_chunk = true;

  const bool md = true;

  const uint8_t expected_msg_all[] = { 0x08, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa, 0x1b, 0x13 };
  prv_check_chunk(&s_chunk_ctx, !md, sizeof(expected_msg_all), &expected_msg_all, sizeof(expected_msg_all));
}

TEST(MemfaultChunkTransport, Test_ChunkerMultiCallSingleChunk) {
  uint8_t s_big_msg[4072] = { 0x1, 0x2, 0x3 };
  s_active_msg = &s_big_msg[0];

  s_chunk_ctx.total_size = MEMFAULT_ARRAY_SIZE(s_big_msg);
  s_chunk_ctx.enable_multi_call_chunk = true;

  // Let's just make sure the sequencer does okay when it takes more than 1 byte to encode a varint
  const bool md = true;
  const uint8_t expected_msg_1[] = { 0x08, 0x1, 0x2, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 };
  prv_check_chunk(&s_chunk_ctx, md, sizeof(expected_msg_1), &expected_msg_1, sizeof(expected_msg_1));

  // read all bytes except for the last one
  uint8_t read_buffer[4063];
  size_t read_buffer_size = sizeof(read_buffer);
  bool md_avail = memfault_chunk_transport_get_next_chunk(&s_chunk_ctx, &read_buffer[0], &read_buffer_size);
  CHECK(md_avail);
  LONGS_EQUAL(sizeof(read_buffer), read_buffer_size);

  const uint8_t expected_msg_2[] = { 0x0, /* crc */ 0xF4, 0x79 };
  prv_check_chunk(&s_chunk_ctx, !md, 20 /* oversize buffer */, &expected_msg_2, sizeof(expected_msg_2));
}
