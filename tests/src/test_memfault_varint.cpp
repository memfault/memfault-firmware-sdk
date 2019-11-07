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

  #include "memfault/util/varint.h"

  static uint8_t s_varint_test_buf[MEMFAULT_UINT32_MAX_VARINT_LENGTH];
}

TEST_GROUP(MemfaultVarint){
  void setup() {
    memset(s_varint_test_buf, 0x0, sizeof(s_varint_test_buf));
  }
  void teardown() {
  }
};

TEST(MemfaultVarint, Test_1byteEncoding) {
  const uint8_t expected_encoding[] = { 127 };

  size_t len = memfault_encode_varint_u32(127, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding), len);
  MEMCMP_EQUAL(expected_encoding, s_varint_test_buf, sizeof(expected_encoding));
}

TEST(MemfaultVarint, Test_2byteEncoding) {
  const uint8_t expected_encoding_min[] = { 128, 1 };

  size_t len = memfault_encode_varint_u32(128, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_min), len);
  MEMCMP_EQUAL(expected_encoding_min, s_varint_test_buf, sizeof(expected_encoding_min));

  const uint8_t expected_encoding_max[] = { 255, 127 };
  len = memfault_encode_varint_u32(16383, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_max), len);
  MEMCMP_EQUAL(expected_encoding_max, s_varint_test_buf, sizeof(expected_encoding_max));
}

TEST(MemfaultVarint, Test_3byteEncoding) {
  const uint8_t expected_encoding_min[] = { 128, 128, 1 };

  size_t len = memfault_encode_varint_u32(16384, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_min), len);
  MEMCMP_EQUAL(expected_encoding_min, s_varint_test_buf, sizeof(expected_encoding_min));

  const uint8_t expected_encoding_max[] = { 255, 255, 127 };
  len = memfault_encode_varint_u32(2097151, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_max), len);
  MEMCMP_EQUAL(expected_encoding_max, s_varint_test_buf, sizeof(expected_encoding_max));
}

TEST(MemfaultVarint, Test_4byteEncoding) {
  const uint8_t expected_encoding_min[] = { 128, 128, 128, 1 };

  size_t len = memfault_encode_varint_u32(2097152, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_min), len);
  MEMCMP_EQUAL(expected_encoding_min, s_varint_test_buf, sizeof(expected_encoding_min));

  const uint8_t expected_encoding_max[] = { 255, 255, 255, 127 };
  len = memfault_encode_varint_u32(268435455, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_max), len);
  MEMCMP_EQUAL(expected_encoding_max, s_varint_test_buf, sizeof(expected_encoding_max));
}

TEST(MemfaultVarint, Test_5byteEncoding) {
  const uint8_t expected_encoding[] = { 239, 253, 182, 245, 13 };

  size_t len = memfault_encode_varint_u32(0xdeadbeef, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding), len);
  MEMCMP_EQUAL(expected_encoding, s_varint_test_buf, sizeof(expected_encoding));

  const uint8_t expected_encoding_max[] = { 255, 255, 255, 255, 15 };
  len = memfault_encode_varint_u32(0xffffffff, s_varint_test_buf);
  LONGS_EQUAL(sizeof(expected_encoding_max), len);
  MEMCMP_EQUAL(expected_encoding_max, s_varint_test_buf, sizeof(expected_encoding_max));
}
