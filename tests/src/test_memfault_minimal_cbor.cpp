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

  #include "memfault/core/math.h"
  #include "memfault/util/cbor.h"


  static void prv_write_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
    CHECK(ctx != NULL);
    uint8_t *result_buf = (uint8_t *)ctx;
    memcpy(&result_buf[offset], buf, buf_len);
  }
}

TEST_GROUP(MemfaultMinimalCbor){
  void setup() {
  }
  void teardown() {
  }
};

static void prv_run_unsigned_integer_encoder_check(
    uint32_t value, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));

  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_unsigned_integer(&encoder, value);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}

TEST(MemfaultMinimalCbor, Test_EncodeUnsignedInt) {

  // RFC Appendix A.  Examples
  const uint8_t expected_enc_0[] =  { 0 };
  prv_run_unsigned_integer_encoder_check(0, expected_enc_0, sizeof(expected_enc_0));

  const uint8_t expected_enc_1[] =  { 1 };
  prv_run_unsigned_integer_encoder_check(1, expected_enc_1, sizeof(expected_enc_1));

  const uint8_t expected_enc_10[] =  { 0x0a };
  prv_run_unsigned_integer_encoder_check(10, expected_enc_10, sizeof(expected_enc_10));

  const uint8_t expected_enc_23[] =  { 0x17 };
  prv_run_unsigned_integer_encoder_check(23, expected_enc_23, sizeof(expected_enc_23));

  const uint8_t expected_enc_24[] =  { 0x18, 0x18 };
  prv_run_unsigned_integer_encoder_check(24, expected_enc_24, sizeof(expected_enc_24));

  const uint8_t expected_enc_25[] =  { 0x18, 0x19 };
  prv_run_unsigned_integer_encoder_check(25, expected_enc_25, sizeof(expected_enc_25));

  const uint8_t expected_enc_100[] =  { 0x18, 0x64 };
  prv_run_unsigned_integer_encoder_check(100, expected_enc_100, sizeof(expected_enc_100));

  const uint8_t expected_enc_1000[] =  { 0x19, 0x03, 0xe8 };
  prv_run_unsigned_integer_encoder_check(1000, expected_enc_1000, sizeof(expected_enc_1000));

  const uint8_t expected_enc_1000000[] = { 0x1a, 0x00, 0x0f, 0x42, 0x40 };
  prv_run_unsigned_integer_encoder_check(1000000, expected_enc_1000000, sizeof(expected_enc_1000000));

  // Also try a UINT32_MAX
  const uint8_t expected_enc_uint32_max[] = { 0x1a, 0xff, 0xff, 0xff, 0xff };
  prv_run_unsigned_integer_encoder_check(UINT32_MAX, expected_enc_uint32_max,
                                 sizeof(expected_enc_uint32_max));
}

static void prv_run_array_encoder_check(
    const uint32_t *values, size_t num_values, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));

  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  bool success = memfault_cbor_encode_array_begin(&encoder, num_values);
  CHECK(success);

  for (size_t i = 0; i < num_values; i++) {
    success = memfault_cbor_encode_unsigned_integer(&encoder, values[i]);
    CHECK(success);
  }

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}

TEST(MemfaultMinimalCbor, Test_Join) {
  // start an encoding for an array
  char result[4];
  memset(result, 0x0, sizeof(result));

  sMemfaultCborEncoder encoder;
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const uint32_t values123[] = { 0x1, 0x2, 0x3 };
  bool success = memfault_cbor_encode_array_begin(&encoder, MEMFAULT_ARRAY_SIZE(values123));
  CHECK(success);

  // build list of integers in a seperate encoder context
  sMemfaultCborEncoder encoder2;
  char cbor_ints[3];
  memfault_cbor_encoder_init(&encoder2, prv_write_cb, cbor_ints, sizeof(cbor_ints));

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(values123); i++) {
    success = memfault_cbor_encode_unsigned_integer(&encoder2, values123[i]);
    CHECK(success);
  }

  size_t encoded_length = memfault_cbor_encoder_deinit(&encoder2);
  LONGS_EQUAL(sizeof(cbor_ints), encoded_length);

  // join results from integer encoding into main encoder
  success = memfault_cbor_join(&encoder, cbor_ints, encoded_length);

  const uint8_t three_num_array_expected_encode[] = { 0x83, 0x01, 0x02, 0x03 };
  encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(sizeof(three_num_array_expected_encode), encoded_length);

  MEMCMP_EQUAL(three_num_array_expected_encode, result, sizeof(three_num_array_expected_encode));
}

TEST(MemfaultMinimalCbor, Test_EncodeArray) {
  // RFC Appendix A. Examples

  // []
  const uint8_t empty_array_expected_encode[] = { 0x80 };
  prv_run_array_encoder_check(NULL, 0, empty_array_expected_encode,
                              sizeof(empty_array_expected_encode));

  // [ 1, 2, 3 ]
  const uint32_t values123[] = { 0x1, 0x2, 0x3 };
  const uint8_t three_num_array_expected_encode[] = { 0x83, 0x01, 0x02, 0x03 };
  prv_run_array_encoder_check(values123, MEMFAULT_ARRAY_SIZE(values123),
                              three_num_array_expected_encode,
                              sizeof(three_num_array_expected_encode));

  // [ 1, 2, 3 .. 25 ]
  const uint32_t longer_array[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25
  };
  const uint8_t longer_array_expected_encode[] = {
      0x98, 0x19, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
      0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19
  };
  prv_run_array_encoder_check(longer_array, MEMFAULT_ARRAY_SIZE(longer_array),
                              longer_array_expected_encode,
                              sizeof(longer_array_expected_encode));
}

static void prv_run_signed_integer_encoder_check(
    int32_t value, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_signed_integer(&encoder, value);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}

TEST(MemfaultMinimalCbor, Test_EncodeSignedInt) {
  // RFC Appendix A.  Examples
  const uint8_t expected_enc_neg1[] =  { 0x20 };
  prv_run_signed_integer_encoder_check(-1, expected_enc_neg1, sizeof(expected_enc_neg1));

  const uint8_t expected_enc_neg10[] =  { 0x29 };
  prv_run_signed_integer_encoder_check(-10, expected_enc_neg10, sizeof(expected_enc_neg10));

  const uint8_t expected_enc_neg100[] =  { 0x38, 0x63 };
  prv_run_signed_integer_encoder_check(-100, expected_enc_neg100, sizeof(expected_enc_neg100));

  const uint8_t expected_enc_neg1000[] =  { 0x39, 0x03, 0xe7 };
  prv_run_signed_integer_encoder_check(-1000, expected_enc_neg1000, sizeof(expected_enc_neg1000));

  // positive values should wind up getting encoded as an unsigned integer
  const uint8_t expected_enc_1000000[] = { 0x1a, 0x00, 0x0f, 0x42, 0x40 };
  prv_run_signed_integer_encoder_check(1000000, expected_enc_1000000, sizeof(expected_enc_1000000));

  // Also try a INT32_MIN
  const uint8_t expected_enc_int32_min[] = { 0x3a, 0x7f, 0xff, 0xff, 0xff };
  prv_run_signed_integer_encoder_check(INT32_MIN, expected_enc_int32_min,
                                 sizeof(expected_enc_int32_min));
}


static void prv_run_long_signed_integer_encoder_check(
    int64_t value, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_long_signed_integer(&encoder, value);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}


TEST(MemfaultMinimalCbor, Test_EncodeLongSignedInt) {
  // We'll re-run a few of the tests for signed ints because the results should be identical
  const uint8_t expected_enc_neg1[] =  { 0x20 };
  prv_run_long_signed_integer_encoder_check(-1, expected_enc_neg1, sizeof(expected_enc_neg1));

  // positive values should wind up getting encoded as an unsigned integer
  const uint8_t expected_enc_1000000[] = { 0x1a, 0x00, 0x0f, 0x42, 0x40 };
  prv_run_long_signed_integer_encoder_check(1000000, expected_enc_1000000, sizeof(expected_enc_1000000));

  const uint8_t expected_enc_int32_min[] = { 0x3a, 0x7f, 0xff, 0xff, 0xff };
  prv_run_long_signed_integer_encoder_check(INT32_MIN, expected_enc_int32_min,
                                            sizeof(expected_enc_int32_min));

  // NB: Anything > INT32_MIN/MAX should wind up being 9 bytes

  // RFC Appendix A.  Examples
  const uint8_t expected_enc_1000000000000[] = { 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 };
  prv_run_long_signed_integer_encoder_check(1000000000000, expected_enc_1000000000000,
                                            sizeof(expected_enc_1000000000000));

  const uint8_t expected_enc_neg123456789101112[] = { 0x3b, 0x00, 0x00, 0x70, 0x48, 0x86, 0x0f, 0x3a, 0x37};
  prv_run_long_signed_integer_encoder_check(-123456789101112, expected_enc_neg123456789101112,
                                            sizeof(expected_enc_neg123456789101112));
}

static void prv_run_string_encoder_check(
    const char *str, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_string(&encoder, str);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}


TEST(MemfaultMinimalCbor, Test_EncodeString) {
  // RFC Appendix A.  Examples
  const uint8_t expected_enc_empty[] =  { 0x60 };
  prv_run_string_encoder_check("", expected_enc_empty, sizeof(expected_enc_empty));

  const uint8_t expected_enc_a[] =  { 0x61, 0x61 };
  prv_run_string_encoder_check("a", expected_enc_a, sizeof(expected_enc_a));

  const uint8_t expected_enc_IETF[] = { 0x64, 0x49, 0x45, 0x54, 0x46 };
  prv_run_string_encoder_check("IETF", expected_enc_IETF, sizeof(expected_enc_IETF));

  const uint8_t expected_enc_quote_backslash[] = { 0x62, 0x22, 0x5c};
  prv_run_string_encoder_check("\"\\", expected_enc_quote_backslash, sizeof(expected_enc_quote_backslash));
}


static void prv_run_incremental_string_encoder_check(
  const char *str, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_string_begin(&encoder, strlen(str));
  CHECK(success);

  for (size_t i = 0; i < strlen(str); ++i) {
    CHECK(memfault_cbor_join(&encoder, &str[i], 1));
  }

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}


TEST(MemfaultMinimalCbor, Test_EncodeStringIncremental) {
  // RFC Appendix A.  Examples
  const uint8_t expected_enc_empty[] =  { 0x60 };
  prv_run_incremental_string_encoder_check("", expected_enc_empty, sizeof(expected_enc_empty));

  const uint8_t expected_enc_a[] =  { 0x61, 0x61 };
  prv_run_incremental_string_encoder_check("a", expected_enc_a, sizeof(expected_enc_a));

  const uint8_t expected_enc_IETF[] = { 0x64, 0x49, 0x45, 0x54, 0x46 };
  prv_run_incremental_string_encoder_check("IETF", expected_enc_IETF, sizeof(expected_enc_IETF));

  const uint8_t expected_enc_quote_backslash[] = { 0x62, 0x22, 0x5c};
  prv_run_incremental_string_encoder_check("\"\\", expected_enc_quote_backslash, sizeof(expected_enc_quote_backslash));
}

static void prv_run_binary_encoder_check(
    const void *buf, size_t buf_len, const uint8_t *expected_seq, size_t expected_seq_len) {

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_byte_string(&encoder, buf, buf_len);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}

TEST(MemfaultMinimalCbor, Test_EncodeBinaryString) {
  // RFC Appendix A.  Examples
  const uint8_t expected_enc_empty[] =  { 0x40 };
  prv_run_binary_encoder_check(NULL, 0, expected_enc_empty, sizeof(expected_enc_empty));

  const uint8_t expected_enc_1234[] = { 0x44, 0x01, 0x02, 0x03, 0x04 };
  const uint8_t binary_str[] = { 1, 2, 3, 4};
  prv_run_binary_encoder_check(binary_str, sizeof(binary_str),
                               expected_enc_1234, sizeof(expected_enc_1234));
}

static void prv_run_uint64_as_double_encoder_check(
    double g, const uint8_t *expected_seq, size_t expected_seq_len) {

  uint64_t value = 0;
  memcpy(&value, &g, sizeof(g));

  sMemfaultCborEncoder encoder;
  uint8_t result[expected_seq_len];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  const bool success = memfault_cbor_encode_uint64_as_double(&encoder, value);
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(expected_seq_len, encoded_length);

  MEMCMP_EQUAL(expected_seq, result, expected_seq_len);
}


TEST(MemfaultMinimalCbor, Test_EncodeFLoatString) {
  // RFC Appendix A.  Examples
  const uint8_t expected_float_pos[] =  { 0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c };

  const double g_pos = 1.0e+300;
  prv_run_uint64_as_double_encoder_check(g_pos, expected_float_pos, sizeof(expected_float_pos));

  const uint8_t expected_float_neg[] =  { 0xfb, 0xc0, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 };
  const double g_neg = -4.1;
  prv_run_uint64_as_double_encoder_check(g_neg, expected_float_neg, sizeof(expected_float_neg));
}

static void prv_encode_test_dictionary(sMemfaultCborEncoder *encoder) {
  // {
  //   "a": "A",
  //   "b": "B",
  //   "c": "C",
  //   "d": "D",
  //   "e": "E"
  // }
  bool success = memfault_cbor_encode_dictionary_begin(encoder, 5);
  CHECK(success);

  const char *enc[] = { "a", "b", "c", "d", "e" };
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(enc); i++) {
    char value[2] = { 0 };
    value[0] = *enc[i] - 32; // get capital representation

    success = memfault_cbor_encode_string(encoder, enc[i]);
    CHECK(success);
    success = memfault_cbor_encode_string(encoder, value);
    CHECK(success);
  }
}

// Real world encoding of a set of data
TEST(MemfaultMinimalCbor, Test_EncodeDictionary) {
  // first let's just compute the size
  sMemfaultCborEncoder encoder;
  memfault_cbor_encoder_size_only_init(&encoder);
  prv_encode_test_dictionary(&encoder);
  const size_t size_needed = memfault_cbor_encoder_deinit(&encoder);

  const uint8_t expected_encoding[] = { 0xa5, 0x61, 0x61, 0x61,
                                      0x41, 0x61, 0x62, 0x61,
                                      0x42, 0x61, 0x63, 0x61,
                                      0x43, 0x61, 0x64, 0x61,
                                      0x44, 0x61, 0x65, 0x61,
                                      0x45 };
  LONGS_EQUAL(sizeof(expected_encoding), size_needed);

  uint8_t result[size_needed];
  memset(result, 0x0, sizeof(result));
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));
  prv_encode_test_dictionary(&encoder);

  MEMCMP_EQUAL(expected_encoding, result, sizeof(result));
}

// nested dictionary
TEST(MemfaultMinimalCbor, Test_EncodeNestedDictionary) {
  // {
  //   "a": {
  //     "b": "c"
  //    }
  // }
  const uint8_t expected_encoding[] = {
    0xa1, 0x61, 0x61, 0xa1,
    0x61, 0x62, 0x61, 0x63
  };

  uint8_t result[sizeof(expected_encoding)];
  memset(result, 0x0, sizeof(result));

  sMemfaultCborEncoder encoder;
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));
  bool success = memfault_cbor_encode_dictionary_begin(&encoder, 1);
  CHECK(success);
  success = memfault_cbor_encode_string(&encoder, "a");
  CHECK(success);
  success = memfault_cbor_encode_dictionary_begin(&encoder, 1);
  CHECK(success);
  success = memfault_cbor_encode_string(&encoder, "b");
  CHECK(success);
  success = memfault_cbor_encode_string(&encoder, "c");
  CHECK(success);

  const size_t encoded_length = memfault_cbor_encoder_deinit(&encoder);
  LONGS_EQUAL(sizeof(result), encoded_length);
  MEMCMP_EQUAL(expected_encoding, result, sizeof(result));
}

TEST(MemfaultMinimalCbor, Test_BufTooSmall) {
  sMemfaultCborEncoder encoder;
  char result[1];
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, sizeof(result));

  // we only have a 1 byte result buffer so a 2 byte encoding should fail
  const bool success = memfault_cbor_encode_string(&encoder, "a");
  CHECK(!success);
}

TEST(MemfaultMinimalCbor, Test_Null) {
  sMemfaultCborEncoder encoder;
  uint8_t result[1];
  memfault_cbor_encoder_init(&encoder, prv_write_cb, result, MEMFAULT_ARRAY_SIZE(result));

  const bool success = memfault_cbor_encode_null(&encoder);
  CHECK(success);

  LONGS_EQUAL(0xF6, result[0]);
}
