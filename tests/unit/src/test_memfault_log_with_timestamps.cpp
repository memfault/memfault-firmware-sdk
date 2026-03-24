//! @file
//!
//! @brief

// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <string.h>
#include <assert.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_platform_metrics_locking.h"
#include "memfault/core/log.h"
#include "memfault/core/log_impl.h"
#include "memfault/core/platform/system_time.h"
#include "memfault_log_data_source_private.h"
#include "memfault_log_private.h"

static bool s_fake_data_source_has_been_triggered;

extern "C" {
bool memfault_log_data_source_has_been_triggered(void) {
  return s_fake_data_source_has_been_triggered;
}
}

TEST_GROUP(MemfaultLogWithTimestamp) {
  void setup() {
    fake_memfault_metrics_platform_locking_reboot();
    s_fake_data_source_has_been_triggered = false;
  }
  void teardown() {
    CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
    memfault_log_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_run_header_check(uint8_t *log_entry, eMemfaultPlatformLogLevel expected_level,
                                 const char *expected_log, size_t expected_log_len,
                                 bool has_timestamp, uint32_t expected_timestamp) {
  // NOTE: Since sMfltRamLogEntry is serialized out, we manually check the header here instead of
  // sharing the struct definition to catch unexpected changes in the layout.
  LONGS_EQUAL(expected_level & 0x03, log_entry[0] & 0x03);
  LONGS_EQUAL(expected_timestamp ? 0x10 : 0, log_entry[0] & 0x10);
  LONGS_EQUAL(expected_log_len & 0xff, log_entry[1]);

  if (has_timestamp) {
    uint32_t timestamp_val;
    memcpy(&timestamp_val, &log_entry[2], sizeof(timestamp_val));
    LONGS_EQUAL(expected_timestamp, timestamp_val);
    MEMCMP_EQUAL(expected_log, &log_entry[2 + 4], expected_log_len - 4);
  } else {
    MEMCMP_EQUAL(expected_log, &log_entry[2], expected_log_len);
  }
}

static void prv_read_log_and_check(eMemfaultPlatformLogLevel expected_level,
                                   eMemfaultLogRecordType expected_type, const void *expected_log,
                                   size_t expected_log_len, uint32_t expected_timestamp) {
  sMemfaultLog log;
  // scribble a bad pattern to make sure memfault_log_read inits things
  memset(&log, 0xa5, sizeof(log));
  const bool found_log = memfault_log_read(&log);
  CHECK(found_log);
  LONGS_EQUAL(expected_log_len, log.msg_len);

  if (expected_type == kMemfaultLogRecordType_Preformatted) {
    STRCMP_EQUAL((const char *)expected_log, log.msg);
  } else {
    MEMCMP_EQUAL(expected_log, log.msg, expected_log_len);
  }

  LONGS_EQUAL(expected_level, log.level);
  LONGS_EQUAL(expected_type, log.type);
  LONGS_EQUAL(expected_timestamp, log.timestamp);
}

TEST(MemfaultLogWithTimestamp, Test_MemfaultLogBasic) {
  uint8_t s_ram_log_store[24];
  CHECK_FALSE(memfault_log_booted());
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  CHECK(memfault_log_booted());

  const char *my_log = "12345678";
  const char *my_log2 = "abcdefgh";
  const size_t my_log_len = strlen(my_log);
  eMemfaultLogRecordType type = kMemfaultLogRecordType_Preformatted;
  sMemfaultCurrentTime time = {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 0x12345678,
    },
  };
  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", (void *)&time, sizeof(time))
    .andReturnValue(false);
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Info, my_log, my_log_len);
  // Write a second log, to exercise the s_memfault_ram_logger.log_read_offset
  // book-keeping:
  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", (void *)&time, sizeof(time))
    .andReturnValue(true);
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Error, my_log2, my_log_len);

  // First log has no timestamp
  prv_run_header_check(s_ram_log_store, kMemfaultPlatformLogLevel_Info, my_log, my_log_len, false,
                       0);
  // Second log has a timestamp
  prv_run_header_check(&s_ram_log_store[10], kMemfaultPlatformLogLevel_Error, my_log2,
                       my_log_len + 4, true, 0x12345678);

  // Read the two logs:
  prv_read_log_and_check(kMemfaultPlatformLogLevel_Info, type, my_log, my_log_len, 0);
  prv_read_log_and_check(kMemfaultPlatformLogLevel_Error, type, my_log2, my_log_len, 0x12345678);

  // should be no more logs
  sMemfaultLog log;
  const bool log_found = memfault_log_read(&log);
  CHECK(!log_found);
}
