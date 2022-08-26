//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <string.h>

#include "memfault/util/base64.h"


TEST_GROUP(MemfaultMinimalCbor){
  void setup() {
  }
  void teardown() {
  }
};

static void prv_test_basic_encoder(const char *input_str, const char *expected_out_str) {
  const size_t in_len = strlen(input_str);
  const size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(in_len);
  char result[encode_len + 1];
  memset(result, 0xA5, sizeof(result));
  result[encode_len] = '\0';
  memfault_base64_encode(input_str, in_len, result);
  STRCMP_EQUAL(expected_out_str, result);
}

static void prv_test_inplace_encoder(const char *input_str, const char *expected_out_str) {
  const size_t in_len = strlen(input_str);
  const size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(in_len);
  char in_out_buf[encode_len + 1];
  memset(in_out_buf, 0xA5, sizeof(in_out_buf));
  in_out_buf[encode_len] = '\0';
  strncpy(in_out_buf, input_str, sizeof(in_out_buf) - 1);
  memfault_base64_encode_inplace(in_out_buf, in_len);
  STRCMP_EQUAL(expected_out_str, in_out_buf);
}

static void prv_run_test_cases(const char *input_str, const char *expected_out_str) {
  prv_test_basic_encoder(input_str, expected_out_str);
  prv_test_inplace_encoder(input_str, expected_out_str);
}

// Test vectors for base64 from:
//   https://tools.ietf.org/html/rfc4648#page-12
TEST(MemfaultMinimalCbor, Test_RfcTestVectors) {
  prv_run_test_cases("", "");
  prv_run_test_cases("f", "Zg==");
  prv_run_test_cases("fo", "Zm8=");
  prv_run_test_cases("foo", "Zm9v");
  prv_run_test_cases("foob", "Zm9vYg==");
  prv_run_test_cases("fooba", "Zm9vYmE=");
  prv_run_test_cases("foobar", "Zm9vYmFy");
}

// Run encoding over all possible values
TEST(MemfaultMinimalCbor, Test_All_BasicEncoder) {
  uint8_t bin_in[256];

  for (size_t i = 0; i < sizeof(bin_in); i++) {
    bin_in[i] = i & 0xff;
  }

  const size_t bin_len = sizeof(bin_in);
  const size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(bin_len);

  char result[encode_len + 1];
  memset(result, 0xA5, sizeof(result));
  result[encode_len] = '\0';

  const char *expected_out_str =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUm"
      "JygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNT"
      "k9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dX"
      "Z3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2"
      "en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TF"
      "xsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7"
      "e7v8PHy8/T19vf4+fr7/P3+/w==";
  memfault_base64_encode(bin_in, bin_len, result);
  STRCMP_EQUAL(expected_out_str, result);
}

TEST(MemfaultMinimalCbor, Test_All_InPlace) {
  const size_t bin_len = 256;
  uint8_t in_out_buf[MEMFAULT_BASE64_ENCODE_LEN(bin_len) + 1];
  const size_t encode_len = sizeof(in_out_buf) - 1;

  memset(in_out_buf, 0xA5, sizeof(in_out_buf));
  in_out_buf[encode_len] = '\0';

  // Fill in-out buffer with binary data to encode
  for (size_t i = 0; i < bin_len; i++) {
    in_out_buf[i] = i & 0xff;
  }

  const char *expected_out_str =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUm"
      "JygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNT"
      "k9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dX"
      "Z3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2"
      "en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TF"
      "xsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7"
      "e7v8PHy8/T19vf4+fr7/P3+/w==";
  memfault_base64_encode_inplace(in_out_buf, bin_len);
  MEMCMP_EQUAL(expected_out_str, in_out_buf, encode_len);
}

TEST(MemfaultMinimalCbor, Test_ConversionMacros) {
  // multiples of 3 should always match

  size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(3);
  LONGS_EQUAL(3, MEMFAULT_BASE64_MAX_DECODE_LEN(encode_len));

  encode_len = MEMFAULT_BASE64_ENCODE_LEN(300);
  LONGS_EQUAL(300, MEMFAULT_BASE64_MAX_DECODE_LEN(encode_len));

  // for non multiples of 3, there will be up to 2 padding characters

  encode_len = MEMFAULT_BASE64_ENCODE_LEN(4);
  LONGS_EQUAL(6, MEMFAULT_BASE64_MAX_DECODE_LEN(encode_len));

  encode_len = MEMFAULT_BASE64_ENCODE_LEN(5);
  LONGS_EQUAL(6, MEMFAULT_BASE64_MAX_DECODE_LEN(encode_len));
}
