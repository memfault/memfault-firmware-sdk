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

#include "fakes/fake_memfault_platform_metrics_locking.h"
#include "fakes/fake_memfault_platform_time.h"

#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/log.h"
#include "memfault/core/log_impl.h"

#include "memfault_log_data_source_private.h"
}

static uint8_t s_ram_log_store[64];

static sMemfaultCurrentTime s_current_time;

static void prv_time_inc(int secs) {
  s_current_time.info.unix_timestamp_secs += (uint64_t)secs;
  fake_memfault_platform_time_set(&s_current_time);
}

TEST_GROUP(MemfaultLogDataSource) {
  void setup() {
    fake_memfault_platform_time_enable(true);
    s_current_time = (sMemfaultCurrentTime) {
      .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
      .info = {
        .unix_timestamp_secs = 0,
      },
    };
    fake_memfault_platform_time_set(&s_current_time);
    memset(s_ram_log_store, 0, sizeof(s_ram_log_store));
    memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
    memfault_log_set_min_save_level(kMemfaultPlatformLogLevel_Debug);
  }
  void teardown() {
    memfault_log_data_source_reset();
    memfault_log_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static size_t prv_add_logs(void) {
  memfault_log_save_preformatted(
    kMemfaultPlatformLogLevel_Debug, "debug", strlen("debug"));
  memfault_log_save_preformatted(
    kMemfaultPlatformLogLevel_Info, "info", strlen("info"));
  memfault_log_save_preformatted(
    kMemfaultPlatformLogLevel_Warning, "warning", strlen("warning"));
  memfault_log_save_preformatted(
    kMemfaultPlatformLogLevel_Error, "error", strlen("error"));
  return 4;
}

static const size_t expected_encoded_size = 59;
static const uint8_t expected_encoded_buffer[expected_encoded_size] = {
  0xA7,
  0x02, 0x04,
  0x03, 0x01,
  0x0A, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1', '.', '2',  '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
  0x01, 0x00,
  0x04, 0x88,
  0x00,
  0x65, 'd', 'e', 'b', 'u', 'g',
  0x01,
  0x64, 'i', 'n', 'f', 'o',
  0x02,
  0x67, 'w', 'a', 'r', 'n', 'i', 'n', 'g',
  0x03,
  0x65, 'e', 'r', 'r', 'o', 'r',
};


TEST(MemfaultLogDataSource, Test_TriggerOnce) {
  prv_add_logs();

  memfault_log_trigger_collection();
  CHECK_TRUE(memfault_log_data_source_has_been_triggered());
  // Idempotent:
  memfault_log_trigger_collection();
}


TEST(MemfaultLogDataSource, Test_TriggerNoopWhenNoLogs) {
  memfault_log_trigger_collection();
  CHECK_FALSE(memfault_log_data_source_has_been_triggered());
}


TEST(MemfaultLogDataSource, Test_TriggerNoopWhenNoUnsentLogs) {
  prv_add_logs();
  memfault_log_trigger_collection();
  g_memfault_log_data_source.mark_msg_read_cb();

  // All logs in the buffer have been marked as sent. Triggering again should be a no-op:
  memfault_log_trigger_collection();
  CHECK_FALSE(memfault_log_data_source_has_been_triggered());
}


TEST(MemfaultLogDataSource, Test_HasMoreMsgsCbNotTriggered) {
  // Buffer contains a log, but memfault_log_trigger_collection() has not been called:
  prv_add_logs();
  size_t size = 0;
  CHECK_FALSE(g_memfault_log_data_source.has_more_msgs_cb(&size));
  LONGS_EQUAL(0, size);
}


TEST(MemfaultLogDataSource, Test_HasMoreMsgs) {
  prv_add_logs();

  memfault_log_trigger_collection();

  size_t size = 0;
  CHECK_TRUE(g_memfault_log_data_source.has_more_msgs_cb(&size));
  LONGS_EQUAL(expected_encoded_size, size);
}


TEST(MemfaultLogDataSource, Test_ReadMsg) {
  prv_add_logs();

  memfault_log_trigger_collection();

  // These logs will not get included, because they happened after the
  // memfault_log_trigger_collection() call:
  prv_add_logs();

  uint8_t cbor_buffer[expected_encoded_size] = {0 };
  for (size_t offset = 0; offset < expected_encoded_size; ++offset) {
    const uint8_t canary = 0x55;
    uint8_t byte[3] = {canary, 0x00, canary};
    CHECK_TRUE(g_memfault_log_data_source.read_msg_cb(offset, &byte[1], 1));

    // Check canary values to detect buffer overruns:
    BYTES_EQUAL(byte[0], canary);
    BYTES_EQUAL(byte[2], canary);
    cbor_buffer[offset] = byte[1];

    // The time encoded in the message should be the time when memfault_log_trigger_collection()
    // was called. Time passing while draining the data source should not affect the message:
    prv_time_inc(131);
  }

  MEMCMP_EQUAL(expected_encoded_buffer, cbor_buffer, expected_encoded_size);
}


TEST(MemfaultLogDataSource, Test_MarkMsgRead) {
  const size_t num_batch_logs_1 = prv_add_logs();
  LONGS_EQUAL(num_batch_logs_1, memfault_log_data_source_count_unsent_logs());

  memfault_log_trigger_collection();

  // These logs will not get included, because they happened after the
  // memfault_log_trigger_collection() call:
  const size_t num_batch_logs_2 = prv_add_logs();
  LONGS_EQUAL(num_batch_logs_1 + num_batch_logs_2,
              memfault_log_data_source_count_unsent_logs());

  g_memfault_log_data_source.mark_msg_read_cb();

  LONGS_EQUAL(num_batch_logs_2, memfault_log_data_source_count_unsent_logs());
}
