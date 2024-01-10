//! @file
//!
//! @brief

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

static bool s_fake_data_source_has_been_triggered;
bool memfault_log_data_source_has_been_triggered(void) {
  return s_fake_data_source_has_been_triggered;
}

TEST_GROUP(MemfaultLog) {
  void setup() {
    fake_memfault_metrics_platorm_locking_reboot();
    s_fake_data_source_has_been_triggered = false;

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

static void prv_run_header_check(uint8_t *log_entry,
                                 eMemfaultPlatformLogLevel expected_level,
                                 const char *expected_log,
                                 size_t expected_log_len) {
  // NOTE: Since sMfltRamLogEntry is serialized out, we manually check the header here instead of
  // sharing the struct definition to catch unexpected changes in the layout.
  LONGS_EQUAL(expected_level & 0x7, log_entry[0]);
  LONGS_EQUAL(expected_log_len & 0xff, log_entry[1]);
  MEMCMP_EQUAL(expected_log, &log_entry[2],expected_log_len);
}

static void prv_read_log_and_check(eMemfaultPlatformLogLevel expected_level,
                                   eMemfaultLogRecordType expected_type,
                                   const void *expected_log, size_t expected_log_len) {
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
}

#if !MEMFAULT_COMPACT_LOG_ENABLE

TEST(MemfaultLog, Test_BadInit) {
  // should be no-ops
  memfault_log_boot(NULL, 10);
  CHECK_FALSE(memfault_log_booted());
  memfault_log_boot((void *)0xbadcafe, 0);
  CHECK_FALSE(memfault_log_booted());

  // calling any API while not enabled should have no effect
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Error, "1", 1);
  MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Error, "%d", 11223344);

  sMemfaultLog log;
  const bool log_found = memfault_log_read(&log);
  CHECK(!log_found);
}

#endif  // !MEMFAULT_COMPACT_LOG_ENABLE

TEST(MemfaultLog, Test_MemfaultLogBasic) {
  uint8_t s_ram_log_store[20];
  CHECK_FALSE(memfault_log_booted());
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  CHECK(memfault_log_booted());

  const char *my_log = "12345678";
  const size_t my_log_len = strlen(my_log);
  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  eMemfaultLogRecordType type = kMemfaultLogRecordType_Preformatted;
  memfault_log_save_preformatted(level, my_log, my_log_len);
  // Write a second log, to exercise the s_memfault_ram_logger.log_read_offset
  // book-keeping:
  memfault_log_save_preformatted(level, my_log, my_log_len);

  prv_run_header_check(s_ram_log_store, level, my_log, my_log_len);
  prv_run_header_check(&s_ram_log_store[10], level, my_log, my_log_len);

  // Read the two logs:
  prv_read_log_and_check(level, type, my_log, my_log_len);
  prv_read_log_and_check(level, type, my_log, my_log_len);

  // should be no more logs
  sMemfaultLog log;
  const bool log_found = memfault_log_read(&log);
  CHECK(!log_found);
}

TEST(MemfaultLog, Test_MemfaultLogOversize) {
  uint8_t s_ram_log_store[MEMFAULT_LOG_MAX_LINE_SAVE_LEN + sizeof(sMfltRamLogEntry)];
  memset(s_ram_log_store, 0, sizeof(s_ram_log_store));
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  char my_log[MEMFAULT_LOG_MAX_LINE_SAVE_LEN + 2];
  memset(my_log, 'A', sizeof(my_log));
  my_log[sizeof(my_log) - 1] = '\0';

  const eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  eMemfaultLogRecordType type = kMemfaultLogRecordType_Preformatted;
  memfault_log_save_preformatted(level, my_log, strlen(my_log));

  my_log[sizeof(my_log) - 2] = '\0';
  prv_read_log_and_check(level, type, my_log, MEMFAULT_LOG_MAX_LINE_SAVE_LEN);
}

void memfault_log_handle_saved_callback(void) {
  mock().actualCall(__func__);
}

TEST(MemfaultLog, Test_MemfaultLog_GetRegions) {
  // try to get regions before init has been called
  sMemfaultLogRegions regions;
  bool found = memfault_log_get_regions(&regions);
  CHECK(!found);

  // now init and confirm we get the expected regions
  uint8_t s_ram_log_store[10];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  found = memfault_log_get_regions(&regions);
  CHECK(found);
  LONGS_EQUAL(s_ram_log_store, regions.region[1].region_start);
  LONGS_EQUAL(sizeof(s_ram_log_store), regions.region[1].region_size);

  // sanity check - first region should be sMfltRamLogger
  const uint8_t *mflt_ram_logger = (const uint8_t *)regions.region[0].region_start;
  LONGS_EQUAL(1, mflt_ram_logger[0]); // version == 1
  LONGS_EQUAL(1, mflt_ram_logger[1]); // enabled == 1
}

#if !MEMFAULT_COMPACT_LOG_ENABLE

TEST(MemfaultLog, Test_MemfaultHandleSaveCallback) {
  uint8_t s_ram_log_store[10];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  mock().enable();
  const char *log0 = "log0";
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Debug,
                                 log0, strlen(log0));
  // should have been filtered so nothing should be called
  mock().checkExpectations();

  mock().expectOneCall("memfault_log_handle_saved_callback");
  MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Info, "log2");
  mock().checkExpectations();

  mock().expectOneCall("memfault_log_handle_saved_callback");
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Warning,
                                 log0, strlen(log0));
  mock().checkExpectations();
}

TEST(MemfaultLog, Test_MemfaultLogTruncation) {
  const size_t max_log_size = MEMFAULT_LOG_MAX_LINE_SAVE_LEN;

  char long_log[max_log_size + 1 + 1 /* \0 */] = { 0 };
  memset(long_log, 'a', sizeof(long_log) - 1);
  LONGS_EQUAL(max_log_size + 1, strlen(long_log));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  uint8_t s_ram_log_store[MEMFAULT_LOG_MAX_LINE_SAVE_LEN * 2];

  // both preformatted and formatted variants should wind up being max size
  memset(s_ram_log_store, 0x0, sizeof(s_ram_log_store));
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  memfault_log_save_preformatted(level, long_log, strlen(long_log));
  prv_run_header_check(s_ram_log_store, level, long_log, MEMFAULT_LOG_MAX_LINE_SAVE_LEN);

  memset(s_ram_log_store, 0x0, sizeof(s_ram_log_store));
  memfault_log_reset();
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  MEMFAULT_LOG_SAVE(level, "%s", long_log);
  prv_run_header_check(s_ram_log_store, level, long_log, MEMFAULT_LOG_MAX_LINE_SAVE_LEN);
}

#endif  // !MEMFAULT_COMPACT_LOG_ENABLE

TEST(MemfaultLog, Test_MemfaultLogExpireOldest) {
  uint8_t s_ram_log_store[10];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *log0 = "0"; // 3 bytes
  const char *log1 = "12"; // 4 bytes with header
  const char *log2 = "45678"; // 7 bytes with header
  const char *log3 = "a"; // 6 bytes with header, should span off 4-9
  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;

  memfault_log_save_preformatted(level, log0, strlen(log0));
  memfault_log_save_preformatted(level, log1, strlen(log1));
  memfault_log_save_preformatted(level, log2, strlen(log2));
  memfault_log_save_preformatted(level, log3, strlen(log3));

  prv_run_header_check(&s_ram_log_store[4], level, log3, strlen(log3));
}

TEST(MemfaultLog, Test_MemfaultLogFrozenBuffer) {
    // Test that when the log buffer is frozen (because the logging data source
    // is using it), logs will not be pruned when a log gets written that does
    // not fit in the remaining space:

    mock().enable();
    mock().expectOneCall("memfault_log_handle_saved_callback");

    uint8_t s_ram_log_store[10];
    memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

    const char *log0 = "45678"; // 7 bytes with header
    eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
    memfault_log_save_preformatted(level, log0, strlen(log0));

    s_fake_data_source_has_been_triggered = true;

    const char *log1 = "abcde"; // 7 bytes with header
    memfault_log_save_preformatted(level, log1, strlen(log1));

    prv_run_header_check(&s_ram_log_store[0], level, log0, strlen(log0));
}

#if !MEMFAULT_COMPACT_LOG_ENABLE

TEST(MemfaultLog, Test_MemfaultLogBufTooLongForStorage) {
  uint8_t s_ram_log_store[5];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const uint8_t magic_pattern = 0xa5;
  memset(s_ram_log_store, magic_pattern, sizeof(s_ram_log_store));

  // a log that is simply too big for storage should not be a no-op
  MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Info, "%s", "more than 5 bytes");
  for (size_t i = 0; i < sizeof(s_ram_log_store); i++) {
    LONGS_EQUAL(magic_pattern, s_ram_log_store[i]);
  }
}

TEST(MemfaultLog, Test_LevelFiltering) {
  uint8_t s_ram_log_store[10] = { 0 };
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *filtered_log = "1234";
  const size_t filtered_log_len = strlen(filtered_log);
  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Debug;
  memfault_log_save_preformatted(level, filtered_log, filtered_log_len);
  MEMFAULT_LOG_SAVE(level, "%s", filtered_log);

  // Enable persisting debug logs
  memfault_log_set_min_save_level(kMemfaultPlatformLogLevel_Debug);
  const char *unfiltered_log = "woo";
  const size_t unfiltered_log_len = strlen(unfiltered_log);
  memfault_log_save_preformatted(level, unfiltered_log, unfiltered_log_len);
  MEMFAULT_LOG_SAVE(level, "%s", unfiltered_log);

  prv_run_header_check(s_ram_log_store, level, unfiltered_log, unfiltered_log_len);
  prv_run_header_check(&s_ram_log_store[5], level, unfiltered_log, unfiltered_log_len);
}

TEST(MemfaultLog, Test_DroppedLogs) {
  uint8_t s_ram_log_store[13] = { 0 };
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  eMemfaultLogRecordType type = kMemfaultLogRecordType_Preformatted;
  const char *initial_log = "hi world!";
  const size_t initial_log_len = strlen(initial_log);
  memfault_log_save_preformatted(level, initial_log, initial_log_len);
  prv_read_log_and_check(level, type, initial_log, initial_log_len);

  for (int i = 0; i < 6; i++) {
    MEMFAULT_LOG_SAVE(level, "MSG %d", i);
  }

  const char *expected_string = "... 5 messages dropped ...";
  prv_read_log_and_check(kMemfaultPlatformLogLevel_Warning, type, expected_string,
                         strlen(expected_string));

  const char *expected_msg5 = "MSG 5";
  prv_read_log_and_check(level, type, expected_msg5, strlen(expected_msg5));
}

#endif  // !MEMFAULT_COMPACT_LOG_ENABLE


static bool prv_log_entry_copy_callback(sMfltLogIterator *iter, size_t offset,
                                        const char *buf, size_t buf_len) {
  return mock()
    .actualCall(__func__)
    .withPointerParameter("iter", iter)
    .withUnsignedLongIntParameter("offset", offset)
    .withConstPointerParameter("buf", buf)
    .withUnsignedLongIntParameter("buf_len", buf_len)
    .returnBoolValueOrDefault(true);
}

static bool prv_iterate_callback(sMfltLogIterator *iter){
  mock().actualCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", iter->read_offset);
  memfault_log_iter_copy_msg(iter, prv_log_entry_copy_callback);
  return true;
}

TEST(MemfaultLog, Test_Iterate) {
  uint8_t s_ram_log_store[10] = {0};
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  const char *log0 = "0";
  const size_t log0_len = strlen(log0);
  memfault_log_save_preformatted(level, log0, log0_len);

  const char *log1 = "1";
  const size_t log1_len = strlen(log1);
  memfault_log_save_preformatted(level, log1, log1_len);

  sMfltLogIterator iterator = (sMfltLogIterator) {0};

  mock().enable();

  mock().expectOneCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", 0);

  mock().expectOneCall("prv_log_entry_copy_callback")
    .withPointerParameter("iter", &iterator)
    .withUnsignedLongIntParameter("offset", 0)
    .withConstPointerParameter("buf", &s_ram_log_store[2])
    .withUnsignedLongIntParameter("buf_len", 1);

  mock().expectOneCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", 3);

  mock().expectOneCall("prv_log_entry_copy_callback")
    .withPointerParameter("iter", &iterator)
    .withUnsignedLongIntParameter("offset", 0)
    .withConstPointerParameter("buf", &s_ram_log_store[5])
    .withUnsignedLongIntParameter("buf_len", 1);

  memfault_log_iterate(prv_iterate_callback, &iterator);
}

TEST(MemfaultLog, Test_LogsExport) {
  uint8_t s_ram_log_store[128] = {0};
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  const char *log0 = "Normal Log";
  const size_t log0_len = strlen(log0);
  memfault_log_save_preformatted(level, log0, log0_len);

  mock().enable();
  mock().expectOneCall("memfault_platform_log_raw").withStringParameter("output", log0);

  memfault_log_export_logs();
  mock().checkExpectations();
}

static jmp_buf s_assert_jmp_buf;

void memfault_sdk_assert_func(void) {
  // we make use of longjmp because this is a noreturn function
  longjmp(s_assert_jmp_buf, -1);
}

TEST(MemfaultLog, Test_LogExportNullLog) {
  uint8_t s_ram_log_store[128] = {0};
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  if (setjmp(s_assert_jmp_buf) == 0) {
    memfault_log_export_log(NULL);
  }
}

#if MEMFAULT_COMPACT_LOG_ENABLE

bool memfault_vlog_compact_serialize(sMemfaultCborEncoder *encoder,
                                     MEMFAULT_UNUSED uint32_t log_id,
                                     MEMFAULT_UNUSED uint32_t compressed_fmt,
                                     MEMFAULT_UNUSED va_list args) {
  uint8_t *mock_compact_log = (uint8_t *)mock().getData("mock_compact_log").getPointerValue();
  size_t mock_compact_log_len = mock().getData("mock_compact_log_len").getUnsignedIntValue();

  return memfault_cbor_join(encoder, mock_compact_log, mock_compact_log_len);
}

TEST(MemfaultLog, Test_CompactLog) {
  uint8_t s_ram_log_store[20];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  eMemfaultLogRecordType type = kMemfaultLogRecordType_Compact;
  uint8_t mock_compact_log[] = {0x01, 0x02, 0x03, 0x04};

  mock().enable();

  // Set mock data for log serialization
  mock().setData("mock_compact_log", mock_compact_log);
  mock().setData("mock_compact_log_len", (int)sizeof(mock_compact_log));
  mock().ignoreOtherCalls();

  memfault_compact_log_save(level, 0, 0);

  // Read the log:
  prv_read_log_and_check(level, type, mock_compact_log, sizeof(mock_compact_log));
}

TEST(MemfaultLog, Test_CompactLogsExport) {
  uint8_t s_ram_log_store[40] = {0};
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  uint8_t mock_compact_log[] = {0x01, 0x02, 0x03, 0x04};

  mock().enable();

  // Set mock data for log serialization
  mock().setData("mock_compact_log", mock_compact_log);
  mock().setData("mock_compact_log_len", (int)sizeof(mock_compact_log));

  // Verify that compact log chunk is output
  mock().expectOneCall("memfault_platform_log_raw").withStringParameter("output", "ML:AQIDBA==:");
  mock().ignoreOtherCalls();

  memfault_compact_log_save(level, 0, 0, 0);

  memfault_log_export_logs();
  mock().checkExpectations();
}

TEST(MemfaultLog, Test_CompactLogsExportMultiChunk) {
  uint8_t s_ram_log_store[40] = {0};
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  // Initialize mock log data twice the size of max chunk to ensure two log chunks
  uint8_t multi_chunk_compact_log[MEMFAULT_LOG_EXPORT_CHUNK_MAX_LEN * 2];
  for (uint8_t i = 0; i < sizeof(multi_chunk_compact_log); i++) {
    multi_chunk_compact_log[i] = i;
  }

  mock().enable();

  // Set mock data for log serialization
  mock().setData("mock_compact_log", multi_chunk_compact_log);
  mock().setData("mock_compact_log_len", (int)sizeof(multi_chunk_compact_log));

  // Verify two compact log chunks are output with correct payload
  mock()
    .expectOneCall("memfault_platform_log_raw")
    .withStringParameter("output", "ML:AAECAwQFBgcICQ==:");
  mock()
    .expectOneCall("memfault_platform_log_raw")
    .withStringParameter("output", "ML:CgsMDQ4PEBESEw==:");
  mock().ignoreOtherCalls();

  memfault_compact_log_save(level, 0, 0, 0);

  memfault_log_export_logs();
  mock().checkExpectations();
}

#endif
