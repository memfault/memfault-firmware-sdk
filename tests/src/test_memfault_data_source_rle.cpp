//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/math.h"

extern "C" {
  #include <string.h>
  #include <stdio.h>
  #include <stddef.h>

  #include "memfault/core/data_source_rle.h"

  static const uint8_t *s_active_data = NULL;
  static size_t s_active_data_size = 0;
}

static bool prv_has_msgs(size_t *total_size_out) {
  *total_size_out = s_active_data_size;
  return (*total_size_out != 0);
}

static bool prv_read_msg_data(uint32_t offset, void *buf, size_t buf_len) {
  memcpy(buf, &s_active_data[offset], buf_len);
  return true;
}

static void prv_mark_msg_read(void) {
  s_active_data = NULL;
  s_active_data_size = 0;
}

static const sMemfaultDataSourceImpl s_test_data_source = {
  .has_more_msgs_cb = prv_has_msgs,
  .read_msg_cb = prv_read_msg_data,
  .mark_msg_read_cb = prv_mark_msg_read,
};

TEST_GROUP(MemfaultDataSourceRle){
  void setup() {
    s_active_data = NULL;
    s_active_data_size = 0;
    memfault_data_source_rle_encoder_set_active(&s_test_data_source);
  }
  void teardown() {
    memfault_data_source_rle_mark_msg_read();
  }
};

TEST(MemfaultDataSourceRle, Test_DataSourceHasMoreMsgs) {
  const uint8_t fake_core[] = { 1, 1 };
  s_active_data = &fake_core[0];
  s_active_data_size = sizeof(fake_core);

  size_t total_size = 0;
  bool more_msgs = memfault_data_source_rle_has_more_msgs(&total_size);
  CHECK(more_msgs);
  LONGS_EQUAL(2, total_size);

  // a re-query shouldn't read more data if its already been computed
  s_active_data = NULL;
  total_size = 0;
  more_msgs = memfault_data_source_rle_has_more_msgs(&total_size);
  CHECK(more_msgs);
  LONGS_EQUAL(2, total_size);

  memfault_data_source_rle_mark_msg_read();
  more_msgs = memfault_data_source_rle_has_more_msgs(&total_size);
  CHECK(!more_msgs);
}

static void prv_get_coredump_data(uint8_t *buf, size_t buf_len, size_t fill_call_size) {
  for (size_t i = 0; i < buf_len; i += fill_call_size) {
    const size_t bytes_to_read = MEMFAULT_MIN(fill_call_size, buf_len - i);
    bool success = memfault_data_source_rle_read_msg(i, &buf[i], bytes_to_read);
    CHECK(success);
  }
}

static void prv_check_pattern(const uint8_t *in, size_t in_len,
                              const uint8_t *expected_out, size_t expected_out_len) {
  // regardless of the size of the buffer we call read_msg_cb() with
  // we should get the same result
  for (size_t fill_size = 1; fill_size < expected_out_len; fill_size++) {
    s_active_data = in;
    s_active_data_size = in_len;

    size_t total_size = 0;
    bool more_msg = memfault_data_source_rle_has_more_msgs(&total_size);
    CHECK(more_msg);
    LONGS_EQUAL(expected_out_len, total_size);

    uint8_t chunk[total_size];
    memset(chunk, 0x0, sizeof(chunk));
    prv_get_coredump_data(chunk, sizeof(chunk), fill_size);

    MEMCMP_EQUAL(expected_out, chunk, expected_out_len);
    memfault_data_source_rle_mark_msg_read();
  }
}

TEST(MemfaultDataSourceRle, Test_DataSourceEndsWithRepeat) {
  const uint8_t fake_core[] = { 1, 1, 2, 3, 4, 5, 5, 5, 5, 5, 6, 9, 9, 9, 9 };
  const uint8_t expected_core_rle[] = {
    4, 1,
    5, 2, 3, 4,
    10, 5,
    1, 6,
    8, 9
  };

  prv_check_pattern(fake_core, sizeof(fake_core), expected_core_rle, sizeof(expected_core_rle));
}

TEST(MemfaultDataSourceRle, Test_DataSourceMultiByteVarintAndEndsWithNonRepeat) {
  uint8_t fake_core[9000] = {
    1, 1, 2, 3, 4, 5, 5, 5, 5, 5, 6, 7,
  };
  fake_core[sizeof(fake_core) - 1] = 8;
  const uint8_t expected_core_rle[] = {
    4, 1,
    5, 2, 3, 4,
    10, 5,
    3, 6, 7,
    182, 140, 1, 0,
    1, 8
  };

  prv_check_pattern(fake_core, sizeof(fake_core), expected_core_rle, sizeof(expected_core_rle));
}
