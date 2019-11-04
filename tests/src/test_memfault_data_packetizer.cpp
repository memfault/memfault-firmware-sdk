//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>
  #include <stdio.h>
  #include <stdint.h>

  #include "memfault/core/data_packetizer.h"
  #include "memfault/core/data_packetizer_source.h"
  #include "memfault/core/math.h"
  #include "memfault/util/chunk_transport.h"

  static uint8_t s_fake_coredump[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa };
  static bool s_multi_call_chunking_enabled = false;

  static uint8_t s_fake_event[] = { 0xa, 0xb, 0xc, 0xd };
}

//
// Mocks & Fakes to exercise packetizer logic
//

static bool prv_coredump_read_core(uint32_t offset, void *buf, size_t buf_len) {
  CHECK((offset + buf_len) <= sizeof(s_fake_coredump));
  memcpy(buf, &s_fake_coredump[offset], buf_len);
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

static void prv_mark_core_read(void) {
  mock().actualCall(__func__);
}

static bool prv_coredump_has_core(size_t *total_size_out) {
  bool has_coredump = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (has_coredump) {
    *total_size_out = sizeof(s_fake_coredump);
  }
  return has_coredump;
}

const sMemfaultDataSourceImpl g_memfault_coredump_data_source = {
  .has_more_msgs_cb = prv_coredump_has_core,
  .read_msg_cb = prv_coredump_read_core,
  .mark_msg_read_cb = prv_mark_core_read,
};

static bool prv_heartbeat_metric_read_event(uint32_t offset, void *buf, size_t buf_len) {
  CHECK((offset + buf_len) <= sizeof(s_fake_event));
  memcpy(buf, &s_fake_event[offset], buf_len);
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

static void prv_heartbeat_metric_mark_read(void) {
  mock().actualCall(__func__);
}

static bool prv_heartbeat_metric_has_event(size_t *total_size_out) {
  // by default disable event
  bool has_coredump = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (has_coredump) {
    *total_size_out = sizeof(s_fake_event);
  }
  return has_coredump;
}

const sMemfaultDataSourceImpl g_memfault_heartbeat_metrics_data_source = {
  .has_more_msgs_cb = prv_heartbeat_metric_has_event,
  .read_msg_cb = prv_heartbeat_metric_read_event,
  .mark_msg_read_cb = prv_heartbeat_metric_mark_read,
};

// For packetizer test purposes, the data within the chunker should be opaque to us
// so just use this fake implementation which simply copies whatever the backing reader
// points to
bool memfault_chunk_transport_get_next_chunk(sMfltChunkTransportCtx *ctx,
                                             void *buf, size_t *buf_len) {
  LONGS_EQUAL(s_multi_call_chunking_enabled, ctx->enable_multi_call_chunk);
  const size_t bytes_to_read = MEMFAULT_MIN(*buf_len, ctx->total_size - ctx->read_offset);
  ctx->read_msg(ctx->read_offset, buf, bytes_to_read);
  ctx->read_offset += bytes_to_read;
  *buf_len = bytes_to_read;
  return (ctx->read_offset != ctx->total_size);
}

void memfault_chunk_transport_get_chunk_info(sMfltChunkTransportCtx *ctx) {
  // fake chunker has 0 overhead so total_chunk_size just matches that
  ctx->single_chunk_message_length = ctx->total_size;
}

TEST_GROUP(MemfaultDataPacketizer){
  void setup() {
    // abort any in-progress transactions
    memfault_packetizer_abort();
    s_multi_call_chunking_enabled = false;
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static bool prv_data_available(void) {
  sPacketizerConfig cfg = {
    .enable_multi_packet_chunk = s_multi_call_chunking_enabled,
  };
  sPacketizerMetadata metadata;
  return memfault_packetizer_begin(&cfg, &metadata);
}

static void prv_begin_transfer(bool data_expected, size_t expected_raw_msg_size) {
 sPacketizerConfig cfg = {
    .enable_multi_packet_chunk = s_multi_call_chunking_enabled,
  };
  sPacketizerMetadata metadata;
  bool md = memfault_packetizer_begin(&cfg, &metadata);
  LONGS_EQUAL(data_expected, md);
  const uint32_t expected_data_len = data_expected ? expected_raw_msg_size + 1 /* hdr */ : 0;
  LONGS_EQUAL(expected_data_len, metadata.single_chunk_message_length);
  CHECK(!metadata.send_in_progress);
}

TEST(MemfaultDataPacketizer, Test_GetPacketNoActiveMessages) {
  uint8_t packet[16];

  mock().expectOneCall("prv_coredump_has_core").andReturnValue(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);

  const bool data_expected = false;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_NoMoreData, rv);

  mock().expectOneCall("prv_coredump_has_core").andReturnValue(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  CHECK(!prv_data_available());
}

static void prv_run_single_packet_test(void) {
  uint8_t packet[16];

  mock().expectOneCall("prv_coredump_has_core");
  mock().expectOneCall("prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_fake_coredump) + 1 /* hdr */, buf_len);
  // packet should be a coredump type
  LONGS_EQUAL(1, packet[0]);
}

static void prv_enable_multi_packet_chunks(void) {
  s_multi_call_chunking_enabled = true;
}

TEST(MemfaultDataPacketizer, Test_MessageFitsInSinglePacket) {
  prv_run_single_packet_test();
}

TEST(MemfaultDataPacketizer, Test_EventMessageFitsInSinglePacket) {
  uint8_t packet[16];

  mock().expectOneCall("prv_coredump_has_core").andReturnValue(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event");
  mock().expectOneCall("prv_heartbeat_metric_read_event");
  mock().expectOneCall("prv_heartbeat_metric_mark_read");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_event));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_fake_event) + 1 /* hdr */, buf_len);
  // packet should be a heartbeat metric type
  LONGS_EQUAL(2, packet[0]);
}

TEST(MemfaultDataPacketizer, Test_MoreDataAvailable) {
  mock().expectNCalls(2, "prv_coredump_has_core");
  CHECK(memfault_packetizer_data_available());
  CHECK(prv_data_available());
  mock().checkExpectations();

  // A message should be batched up after the first check so there
  // should be no new call to has_valid_coredump
  CHECK(prv_data_available());
  CHECK(memfault_packetizer_data_available());

  uint8_t packet[16];
  mock().expectOneCall("prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_fake_coredump) + 1 /* hdr */, buf_len);
  // packet should be a coredump type
  LONGS_EQUAL(1, packet[0]);
}

TEST(MemfaultDataPacketizer, Test_MultiPacketChunking) {
  // when multi chunk packets have been configured, we just need
  // to make sure that the setting is in the chunk context
  prv_enable_multi_packet_chunks();
  prv_run_single_packet_test();
}

static void prv_test_msg_fits_in_multiple_packets(void) {
  uint8_t packet[2];
  const size_t num_calls = (sizeof(s_fake_coredump) + sizeof(packet)) / sizeof(packet);

  mock().expectOneCall("prv_coredump_has_core");
  mock().expectNCalls(num_calls, "prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  // the fake chunker has 0 overhead
  size_t total_packet_length = sizeof(s_fake_coredump) + 1 /* hdr */;
  for (size_t i = 0; i < num_calls; i++) {
    size_t buf_len = sizeof(packet);
    eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
    const size_t expected_buf_len = MEMFAULT_MIN(total_packet_length, sizeof(packet));
    LONGS_EQUAL(expected_buf_len, buf_len);
    total_packet_length -= buf_len;
    if (i == 0) {
      // packet should be a coredump type
      LONGS_EQUAL(1, packet[0]);
    }

    if ((i != (num_calls - 1)) && s_multi_call_chunking_enabled) {
      LONGS_EQUAL(kMemfaultPacketizerStatus_MoreDataForChunk, rv);
    } else {
      LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
    }
  }
}

TEST(MemfaultDataPacketizer, Test_MessageFitsInMultiplePackets) {
  prv_enable_multi_packet_chunks();
  prv_test_msg_fits_in_multiple_packets();
}

TEST(MemfaultDataPacketizer, Test_OneChunkMultiplePackets) {
  prv_test_msg_fits_in_multiple_packets();
}

TEST(MemfaultDataPacketizer, Test_MessageSendAbort) {
  uint8_t packet[5];

  // start sending a packet and abort after we have received one packet
  // we should re-wind and see the entire message transmitted again
  mock().expectOneCall("prv_coredump_has_core");
  mock().expectOneCall("prv_coredump_read_core");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(5, buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
  memfault_packetizer_abort();
  mock().checkExpectations();

  prv_test_msg_fits_in_multiple_packets();
}

TEST(MemfaultDataPacketizer, Test_MessageWithCoredumpReadFailure) {
  uint8_t packet[16];

  mock().expectOneCall("prv_coredump_has_core");
  mock().expectOneCall("prv_coredump_read_core").andReturnValue(false);
  mock().expectOneCall("prv_mark_core_read");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_fake_coredump) + 1 /* hdr */, buf_len);
  // packet should be a coredump type
  LONGS_EQUAL(1, packet[0]);
}

TEST(MemfaultDataPacketizer, Test_MessageOnlyHdrFits) {
  // NOTE: for the fake transport, the chunker has zero overhead
  uint8_t packet_only_hdr[1];
  mock().expectOneCall("prv_coredump_has_core");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet_only_hdr);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet_only_hdr, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
  LONGS_EQUAL(sizeof(packet_only_hdr), buf_len);
  LONGS_EQUAL(1, packet_only_hdr[0]);
  mock().checkExpectations();

  // if we call memfault_packetizer_begin() while in the middle of sending a message, there should
  // we should see that a send is in progress
  sPacketizerConfig cfg = {
    .enable_multi_packet_chunk = s_multi_call_chunking_enabled,
  };
  sPacketizerMetadata metadata;
  bool md = memfault_packetizer_begin(&cfg, &metadata);
  LONGS_EQUAL(data_expected, md);
  const uint32_t expected_data_len = sizeof(s_fake_coredump) + 1 /* hdr */;
  LONGS_EQUAL(expected_data_len, metadata.single_chunk_message_length);
  CHECK(metadata.send_in_progress);

  // now read the actual coredump data
  mock().expectOneCall("prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");
  uint8_t packet[16];
  buf_len = sizeof(packet);
  rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
  LONGS_EQUAL(sizeof(s_fake_coredump), buf_len);
}

// exercise the path where a zero length buffer is passed
TEST(MemfaultDataPacketizer, Test_ZeroLengthBuffer) {
  uint8_t packet[5];

  // start sending a packet and abort after we have received one packet
  // we should re-wind and see the entire message transmitted again
  mock().expectOneCall("prv_coredump_has_core");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = 0;
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(0, buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
}

TEST(MemfaultDataPacketizer, Test_BadArguments) {
  uint8_t packet[5];
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, NULL);
  LONGS_EQUAL(kMemfaultPacketizerStatus_NoMoreData, rv);

  size_t buf_len = 0;
  rv = memfault_packetizer_get_next(NULL, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_NoMoreData, rv);

  sPacketizerConfig cfg = { 0 };
  sPacketizerMetadata metadata;
  bool md = memfault_packetizer_begin(NULL, &metadata);
  CHECK(!md);
  md = memfault_packetizer_begin(&cfg, NULL);
  CHECK(!md);
}
