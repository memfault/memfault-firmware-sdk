//! @file
//!
//! Unit tests for memfault_compact_log_save() when serialized entry exceeds
//! MEMFAULT_LOG_MAX_LINE_SAVE_LEN

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_platform_metrics_locking.h"
#include "memfault/core/compact_log_serializer.h"
#include "memfault/core/log.h"
#include "memfault/core/log_impl.h"
#include "memfault/core/sdk_assert.h"
#include "memfault_log_data_source_private.h"
#include "memfault_log_private.h"

extern "C" {
// emulated linker symbol expected by the serializer
extern uint32_t __start_log_fmt;
uint32_t __start_log_fmt;

bool memfault_log_data_source_has_been_triggered(void) {
  return false;
}

void memfault_sdk_assert_func(void) { }
}

TEST_GROUP(MemfaultCompactLogSaveTruncation) {
  void setup() {
    fake_memfault_metrics_platform_locking_reboot();

    // we selectively enable memfault_log_handle_saved_callback() for certain tests
    mock().disable();
  }
  void teardown() {
    CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
    memfault_log_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static uint32_t prv_get_fake_log_id(void) {
  // We'll just force all our log ids to be the same for test purposes. We just want to verify that
  // its computed relative to the "__start_log_fmt" linker symbol
  return (uint32_t)(uintptr_t)&__start_log_fmt + 0xA;
}

static eMemfaultPlatformLogLevel prv_record_oversized_compact_log(void) {
  const eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  const uint32_t log_id = prv_get_fake_log_id();

  // Create a compact log format that will result in a very long serialized output
  // This uses many string arguments to exceed MEMFAULT_LOG_MAX_LINE_SAVE_LEN (32 bytes)
  const uint32_t compressed_fmt = 0x1FFF;  // 0b1.1111.1111.1111 (12 string args)

#define LONG_STRING "This is a very long string that will make the compact log exceed the limit"

  // Call memfault_compact_log_save with many long string arguments
  memfault_compact_log_save(level, log_id, compressed_fmt, LONG_STRING, LONG_STRING, LONG_STRING,
                            LONG_STRING, LONG_STRING, LONG_STRING, LONG_STRING, LONG_STRING,
                            LONG_STRING, LONG_STRING, LONG_STRING, LONG_STRING);

  return level;
}

TEST(MemfaultCompactLogSaveTruncation, Test_CompactLogSaveWithFallback) {
  // Set up a buffer large enough to hold the fallback entry but not the original oversized entry
  uint8_t s_ram_log_store[128];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = prv_record_oversized_compact_log();

  // The function should have created a fallback entry instead of the original log
  sMemfaultLog log;
  const bool found_log = memfault_log_read(&log);
  CHECK(found_log);

  // Verify it's a compact log type
  LONGS_EQUAL(kMemfaultLogRecordType_Compact, log.type);
  LONGS_EQUAL(level, log.level);

  // The fallback entry should contain:
  // [<log_id_offset>, {0:1, 1:serialized_len}]
  const uint8_t expected_cbor[] = {
    0x82,                   // array of 2 elements
    0x0A,                   // log_id offset (0xA)
    0xA2,                   // map of 2 elements
    0x00, 0x01,             // key 0 -> value 1 (fallback reason)
    0x01, 0x19, 0x01, 0xCA  // key 1 -> value 458 (serialized_len)
  };
  LONGS_EQUAL(sizeof(expected_cbor), log.msg_len);  // array of 2 elements
  MEMCMP_EQUAL(expected_cbor, log.msg, sizeof(expected_cbor));

  // Verify there are no more logs (original oversized log should not be saved)
  const bool found_second_log = memfault_log_read(&log);
  CHECK_FALSE(found_second_log);
}

// test logging when the total ram log storage is too small to hold even the fallback entry
TEST(MemfaultCompactLogSaveTruncation, Test_CompactLogSaveWithFallback_TooSmallStorage) {
  // Set up a buffer too small to hold even the fallback entry
  uint8_t s_ram_log_store[5];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  (void)prv_record_oversized_compact_log();

  // We should see the dropped log message response
  sMemfaultLog log;
  const bool found_log = memfault_log_read(&log);
  CHECK(found_log);
  LONGS_EQUAL(kMemfaultLogRecordType_Preformatted, log.type);
  LONGS_EQUAL(kMemfaultPlatformLogLevel_Warning, log.level);
  STRCMP_EQUAL("... 1 messages dropped ...", log.msg);
}
