//! @file
//!
//! @brief

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "memfault/core/data_packetizer.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/data_source_rle.h"
#include "memfault/core/math.h"
#include "memfault/util/chunk_transport.h"

static uint8_t s_fake_coredump[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xa};
static bool s_multi_call_chunking_enabled = false;
static uint8_t s_fake_event[] = {0xa, 0xb, 0xc, 0xd};
static const sMemfaultDataSourceImpl *s_active_rle_data_source = NULL;


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

const sMemfaultDataSourceImpl g_memfault_event_data_source = {
  .has_more_msgs_cb = prv_heartbeat_metric_has_event,
  .read_msg_cb = prv_heartbeat_metric_read_event,
  .mark_msg_read_cb = prv_heartbeat_metric_mark_read,
};

bool memfault_data_source_rle_encoder_set_active(const sMemfaultDataSourceImpl *active_source) {
  s_active_rle_data_source = active_source;
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}


static bool prv_rle_read_data(uint32_t offset, void *buf, size_t buf_len) {
  mock().actualCall(__func__);
  return s_active_rle_data_source->read_msg_cb(offset, buf, buf_len);
}

static void prv_rle_mark_msg_read(void) {
  mock().actualCall(__func__);
  return s_active_rle_data_source->mark_msg_read_cb();
}

static bool prv_rle_has_msg(size_t *total_size_out) {
  mock().actualCall(__func__);
  return s_active_rle_data_source->has_more_msgs_cb(total_size_out);
}

const sMemfaultDataSourceImpl g_memfault_data_rle_source = {
  .has_more_msgs_cb = prv_rle_has_msg,
  .read_msg_cb = prv_rle_read_data,
  .mark_msg_read_cb = prv_rle_mark_msg_read,
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

static const char *s_log_scope = "log_data_source";
static const char *s_cdr_scope = "cdr_source";

TEST_GROUP(MemfaultDataPacketizer){
  void setup() {
    // abort any in-progress transactions
    mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
    memfault_packetizer_abort();
    mock().checkExpectations();
    mock().clear();
    s_multi_call_chunking_enabled = false;
    mock().strictOrder();
    mock(s_log_scope).disable();
    mock(s_cdr_scope).disable();
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

static void prv_setup_expect_coredump_call_expectations(bool has_core) {
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active").andReturnValue(false);
  mock().expectOneCall("prv_coredump_has_core").andReturnValue(has_core);
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

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);

  const bool data_expected = false;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_NoMoreData, rv);

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  CHECK(!prv_data_available());
}

static void prv_run_single_packet_test(void) {
  uint8_t packet[16];

  prv_setup_expect_coredump_call_expectations(true);
  mock().expectOneCall("prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

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

TEST(MemfaultDataPacketizer, Test_MessageUsesRle) {
  // For unit tests we've mocked out rle to return the same data as the active source
  // What we verify is:
  //   1. The type bit is correctly set to indicate RLE was used
  //   2. All data source accesses route through the rle data source
  uint8_t packet[16];

  mock().expectOneCall("memfault_data_source_rle_encoder_set_active").andReturnValue(true);
  mock().expectOneCall("prv_rle_has_msg");
  mock().expectOneCall("prv_coredump_has_core").andReturnValue(true);

  mock().expectOneCall("prv_rle_read_data");
  mock().expectOneCall("prv_coredump_read_core");

  mock().expectOneCall("prv_rle_mark_msg_read");
  mock().expectOneCall("prv_mark_core_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_fake_coredump) + 1 /* hdr */, buf_len);
  // packet should be a coredump type with RLE bit set
  LONGS_EQUAL(1 | 0x80, packet[0]);
}

TEST(MemfaultDataPacketizer, Test_EventMessageFitsInSinglePacket) {
  uint8_t packet[16];

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event");
  mock().expectOneCall("prv_heartbeat_metric_read_event");
  mock().expectOneCall("prv_heartbeat_metric_mark_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

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
  for (int i = 0; i < 2; i++) {
    prv_setup_expect_coredump_call_expectations(true);
  }

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
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

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

  prv_setup_expect_coredump_call_expectations(true);
  mock().expectNCalls(num_calls, "prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

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

TEST(MemfaultDataPacketizer, Test_SimpleGetChunkApi) {
  uint8_t packet[2];
  bool got_data;
  const size_t num_calls = (sizeof(s_fake_coredump) + sizeof(packet)) / sizeof(packet);

  prv_setup_expect_coredump_call_expectations(true);
  mock().expectNCalls(num_calls, "prv_coredump_read_core");
  mock().expectOneCall("prv_mark_core_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

  // the fake chunker has 0 overhead
  size_t total_packet_length = sizeof(s_fake_coredump) + 1 /* hdr */;
  for (size_t i = 0; i < num_calls; i++) {
    size_t buf_len = sizeof(packet);
    got_data = memfault_packetizer_get_chunk(packet, &buf_len);
    CHECK(got_data);
    const size_t expected_buf_len = MEMFAULT_MIN(total_packet_length, sizeof(packet));
    LONGS_EQUAL(expected_buf_len, buf_len);
    total_packet_length -= buf_len;
    if (i == 0) {
      // packet should be a coredump type
      LONGS_EQUAL(1, packet[0]);
    }
  }
  mock().checkExpectations();

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  size_t buf_len = sizeof(packet);
  got_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!got_data);
}

TEST(MemfaultDataPacketizer, Test_MessageSendAbort) {
  uint8_t packet[5];

  // start sending a packet and abort after we have received one packet
  // we should re-wind and see the entire message transmitted again
  prv_setup_expect_coredump_call_expectations(true);
  mock().expectOneCall("prv_coredump_read_core");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(5, buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_abort();
  mock().checkExpectations();

  prv_test_msg_fits_in_multiple_packets();
}

TEST(MemfaultDataPacketizer, Test_MessageWithCoredumpReadFailure) {
  uint8_t packet[16];

  prv_setup_expect_coredump_call_expectations(true);
  mock().expectOneCall("prv_coredump_read_core").andReturnValue(false);
  mock().expectOneCall("prv_mark_core_read");
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

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
  prv_setup_expect_coredump_call_expectations(true);

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
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  uint8_t packet[16];
  buf_len = sizeof(packet);
  rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);
  LONGS_EQUAL(sizeof(s_fake_coredump), buf_len);
}

// exercise the path where a zero length buffer is passed
TEST(MemfaultDataPacketizer, Test_ZeroLengthBuffer) {
  uint8_t packet[0];

  // start sending a packet and abort after we have received one packet
  // we should re-wind and see the entire message transmitted again
  prv_setup_expect_coredump_call_expectations(true);

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_fake_coredump));

  size_t buf_len = 0;
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(0, buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_NoMoreData, rv);
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

static const size_t LOG_SIZE = 1;

static bool prv_has_logs(size_t *size) {
  const bool has_logs = mock(s_log_scope).actualCall(__func__).returnBoolValueOrDefault(false);
  if (has_logs) {
    *size = LOG_SIZE;
  }
  return has_logs;
}

static bool prv_logs_read(uint32_t offset, void *buf, size_t buf_len) {
  (void)offset;
  memset(buf, 'A', buf_len);
  mock(s_log_scope).actualCall(__func__);
  return true;
}

static void prv_logs_mark_sent(void) {
  mock(s_log_scope).actualCall(__func__);
}

const sMemfaultDataSourceImpl g_memfault_log_data_source  = {
    .has_more_msgs_cb = prv_has_logs,
    .read_msg_cb = prv_logs_read,
    .mark_msg_read_cb = prv_logs_mark_sent,
};

TEST(MemfaultDataPacketizer, Test_LogSourceIsHookedUp) {
  uint8_t packet[16];

  mock(s_log_scope).enable();

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

  mock(s_log_scope).expectOneCall("prv_has_logs").andReturnValue(true);
  mock(s_log_scope).expectOneCall("prv_logs_read");
  mock(s_log_scope).expectOneCall("prv_logs_mark_sent");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, LOG_SIZE);

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(LOG_SIZE + 1 /* hdr */, buf_len);
  // packet should be a log type
  BYTES_EQUAL(3, packet[0]);
  BYTES_EQUAL('A', packet[1]);
}

static const uint8_t s_cdr_payload[] = { 0x1, 0x2, 0x3, 0x4 };

static bool prv_has_cdr(size_t *size) {
  const bool has_cdr = mock(s_cdr_scope).actualCall(__func__).returnBoolValueOrDefault(false);
  if (has_cdr) {
    *size = sizeof(s_cdr_payload);
  }
  return has_cdr;
}

static bool prv_cdr_read(uint32_t offset, void *buf, size_t buf_len) {
  memcpy(buf, &s_cdr_payload[offset], buf_len);
  mock(s_cdr_scope).actualCall(__func__);
  return true;
}

static void prv_cdr_mark_sent(void) {
  mock(s_cdr_scope).actualCall(__func__);
}

const sMemfaultDataSourceImpl g_memfault_cdr_source  = {
  .has_more_msgs_cb = prv_has_cdr,
  .read_msg_cb = prv_cdr_read,
  .mark_msg_read_cb = prv_cdr_mark_sent,
};

TEST(MemfaultDataPacketizer, Test_CdrSourceIsHookedUp) {
  uint8_t packet[16];

  mock(s_cdr_scope).enable();

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

  mock(s_cdr_scope).expectOneCall("prv_has_cdr").andReturnValue(true);
  mock(s_cdr_scope).expectOneCall("prv_cdr_read");
  mock(s_cdr_scope).expectOneCall("prv_cdr_mark_sent");

  const bool data_expected = true;
  prv_begin_transfer(data_expected, sizeof(s_cdr_payload));

  size_t buf_len = sizeof(packet);
  eMemfaultPacketizerStatus rv = memfault_packetizer_get_next(packet, &buf_len);
  LONGS_EQUAL(kMemfaultPacketizerStatus_EndOfChunk, rv);

  // the fake chunker has 0 overhead
  LONGS_EQUAL(sizeof(s_cdr_payload) + 1 /* hdr */, buf_len);
  // packet should be a log type
  BYTES_EQUAL(4, packet[0]);
  MEMCMP_EQUAL(s_cdr_payload, &packet[1], sizeof(s_cdr_payload));
}

TEST(MemfaultDataPacketizer, Test_ActiveSources) {
  uint8_t packet[16];

  // changing the sources will abort any in-progress transmissions
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_None);
  mock().checkExpectations();

  size_t buf_len = sizeof(packet);
  bool more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // exclusively enable coredumps and we should only see the coredump source get called
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_Coredump);

  prv_setup_expect_coredump_call_expectations(false);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // exclusively enable events and we should only see the coredump source get called
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_Event);

  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // exclusively enabled logs and we should only see the log source get called
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_Log);

  mock(s_log_scope).expectOneCall("prv_has_logs").andReturnValue(true);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // exclusively enabled cdr and we should only see the log source get called
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_Cdr);

  mock(s_log_scope).expectOneCall("prv_has_cdr").andReturnValue(true);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // events + logs enabled
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");

  // note the cast to eMfltDataSourceMask
  memfault_packetizer_set_active_sources(
    (kMfltDataSourceMask_Event | kMfltDataSourceMask_Log));
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  mock(s_log_scope).expectOneCall("prv_has_logs").andReturnValue(true);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();

  // if all sources are enabled, they should all be checked
  mock().expectOneCall("memfault_data_source_rle_encoder_set_active");
  memfault_packetizer_set_active_sources(kMfltDataSourceMask_All);

  prv_setup_expect_coredump_call_expectations(false);
  mock().expectOneCall("prv_heartbeat_metric_has_event").andReturnValue(false);
  mock(s_log_scope).expectOneCall("prv_has_logs").andReturnValue(true);
  more_data = memfault_packetizer_get_chunk(packet, &buf_len);
  CHECK(!more_data);
  mock().checkExpectations();
}

TEST(MemfaultDataPacketizer, Test_ZeroBufferLength) {
  // AddressSanitizer will trip on this if anything is written to it
  uint8_t empty_buf[0];
  size_t buf_len = 0;
  prv_setup_expect_coredump_call_expectations(true);
  bool rv = memfault_packetizer_get_chunk(empty_buf, &buf_len);
  LONGS_EQUAL(false, rv);
  LONGS_EQUAL(0, buf_len);
  mock().checkExpectations();
}

TEST(MemfaultDataPacketizer, Test_MinimalBufferLength) {
  // the fake chunker has 0 overhead, so the minimal chunk size here is 1 byte.
  uint8_t small_buf[1];
  size_t buf_len = sizeof(small_buf);
  prv_setup_expect_coredump_call_expectations(true);
  bool rv = memfault_packetizer_get_chunk(small_buf, &buf_len);
  LONGS_EQUAL(1, buf_len);
  LONGS_EQUAL(true, rv);
  mock().checkExpectations();
}
