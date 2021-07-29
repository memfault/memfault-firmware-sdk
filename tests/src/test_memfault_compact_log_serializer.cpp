//! @file
//!

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stdlib.h>


#include "memfault/core/compact_log_serializer.h"

extern "C" {
  // emulated linker symbol expected by the serializer
  extern uint32_t __start_log_fmt;
  uint32_t __start_log_fmt;

  static void prv_write_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
    CHECK(ctx != NULL);
    uint8_t *result_buf = (uint8_t *)ctx;
    memcpy(&result_buf[offset], buf, buf_len);
  }
}

TEST_GROUP(MfltCompactLog) {
  void setup() {
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static uint32_t prv_get_fake_log_id(void) {
  // We'll just force all our log ids to be the same for test purposes. We just want to verify that
  // its computed relative to the "__start_log_fmt" linker symbol
  return (uint32_t)(uintptr_t)&__start_log_fmt + 0xA;
}

static char *prv_setup_encoder(sMemfaultCborEncoder *encoder, size_t expected_size) {
  char *buf = (char *)calloc(expected_size, 1);
  memfault_cbor_encoder_init(encoder, prv_write_cb, buf, expected_size);
  return buf;
}

static void prv_check_result(sMemfaultCborEncoder *encoder, char *result,
                             const void *expected_buf, size_t expected_buf_len) {
  const size_t encoded_length = memfault_cbor_encoder_deinit(encoder);
  LONGS_EQUAL(expected_buf_len, encoded_length);
  MEMCMP_EQUAL(expected_buf, result, expected_buf_len);
  free(result);
}

TEST(MfltCompactLog, Test_MfltCompactLog_NoArg) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {0x81, 0x0A};
  const size_t expected_seq_len = sizeof(expected_seq);

  char *result_buf = prv_setup_encoder(&encoder, expected_seq_len);

  const uint32_t compressed_fmt = 1; // no args
  memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(), compressed_fmt);
  prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
}

TEST(MfltCompactLog, Test_MfltCompactLog_Int32) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {0x82, 0x0A, 0xB};
  const size_t expected_seq_len = sizeof(expected_seq);

  char *result_buf = prv_setup_encoder(&encoder, expected_seq_len);

  const uint32_t compressed_fmt = 0x4; // 0b100
  const bool success = memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(), compressed_fmt, 11);
  CHECK(success);
  prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
}

TEST(MfltCompactLog, Test_MfltCompactLog_Int64) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {0x82, 0x0A, 0x81, 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00};
  const size_t expected_seq_len = sizeof(expected_seq);

  char *result_buf = prv_setup_encoder(&encoder, expected_seq_len);

  const uint32_t compressed_fmt = 0x5; // 0b101
  const int64_t bigint = 1000000000000;
  const bool success = memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(), compressed_fmt, bigint);
  CHECK(success);
  prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
}

TEST(MfltCompactLog, Test_MfltCompactLog_Double) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {0x82, 0x0A, 0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c};
  const size_t expected_seq_len = sizeof(expected_seq);

  char *result_buf = prv_setup_encoder(&encoder, expected_seq_len);

  const uint32_t compressed_fmt = 0x6; // 0b110
  const double a_double = 1.0e+300;
  const bool success = memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(), compressed_fmt, a_double);
  CHECK(success);
  prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
}

TEST(MfltCompactLog, Test_MfltCompactLog_String) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {0x82, 0x0A, 0x65, 'h', 'e', 'l', 'l', 'o'};
  const size_t expected_seq_len = sizeof(expected_seq);

  char *result_buf = prv_setup_encoder(&encoder, expected_seq_len);

  const uint32_t compressed_fmt = 0x7; // 0b111
  const char *str_arg = "hello";
  const bool success = memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(), compressed_fmt, str_arg);
  CHECK(success);
  prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
}


TEST(MfltCompactLog, Test_MfltCompactLog_MultiArg) {
  sMemfaultCborEncoder encoder;

  uint8_t expected_seq[] = {
    0x85, 0x0A,
    0xB,
    0x81, 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00,
    0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c,
    0x65, 'h', 'e', 'l', 'l', 'o'
  };
  const size_t expected_seq_len = sizeof(expected_seq);

  const uint32_t compressed_fmt = 0x11B; // 0b1.0001.1011

  for (size_t i = 0; i <= expected_seq_len; i++) {
    char *result_buf = prv_setup_encoder(&encoder, i);

    // all together
    const uint8_t hhx = 11;
    const int64_t lld = 1000000000000;
    const double g = 1.0e+300;
    const char *str_arg = "hello";
    const bool success = memfault_log_compact_serialize(&encoder, prv_get_fake_log_id(),
                                                compressed_fmt,
                                                hhx, lld, g, str_arg);
    const bool success_expected = (i == expected_seq_len);
    CHECK(success == success_expected);
    if (success) {
      prv_check_result(&encoder, result_buf, expected_seq, expected_seq_len);
    } else {
      free(result_buf);
    }
  }
}
