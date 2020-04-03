//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fakes/fake_memfault_platform_metrics_locking.h"

#include "memfault/core/log.h"

TEST_GROUP(MemfaultEventStorage) {
  void setup() {
    fake_memfault_metrics_platorm_locking_reboot();
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

TEST(MemfaultEventStorage, Test_BadInit) {
  // should be no-ops
  memfault_log_boot(NULL, 10);
  memfault_log_boot((void*)0xbadcafe, 0);

  // calling any API while not enabled should have no effect
  memfault_log_save_preformatted(kMemfaultPlatformLogLevel_Error, "1", 1);
  MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Error, "%d", 11223344);
}

TEST(MemfaultEventStorage, Test_MemfaultLogBasic) {
  uint8_t s_ram_log_store[10];
  memfault_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *my_log = "12345678";
  const size_t my_log_len = strlen(my_log);
  eMemfaultPlatformLogLevel level = kMemfaultPlatformLogLevel_Info;
  memfault_log_save_preformatted(level, my_log, my_log_len);

  prv_run_header_check(s_ram_log_store, level, my_log, my_log_len);
}

TEST(MemfaultEventStorage, Test_MemfaultLogTruncation) {
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

TEST(MemfaultEventStorage, Test_MemfaultLogExpireOldest) {
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

TEST(MemfaultEventStorage, Test_MemfaultLogBufTooLongForStorage) {
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

TEST(MemfaultEventStorage, Test_LevelFiltering) {
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
