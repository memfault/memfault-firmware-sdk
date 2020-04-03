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

static void prv_run_test_case(const char *input_str, const char *expected_out_str) {
  const size_t in_len = strlen(input_str);
  const size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(in_len);
  char result[encode_len + 1];
  memset(result, 0xA5, sizeof(result));
  result[encode_len] = '\0';
  memfault_base64_encode(input_str, in_len, result);
  STRCMP_EQUAL(expected_out_str, result);
}

// Test vectors for base64 from:
//   https://tools.ietf.org/html/rfc4648#page-12
TEST(MemfaultMinimalCbor, Test_RfcTestVectors) {
  prv_run_test_case("", "");
  prv_run_test_case("f", "Zg==");
  prv_run_test_case("fo", "Zm8=");
  prv_run_test_case("foo", "Zm9v");
  prv_run_test_case("foob", "Zm9vYg==");
  prv_run_test_case("fooba", "Zm9vYmE=");
  prv_run_test_case("foobar", "Zm9vYmFy");
}

// Run encoding over all possible values
TEST(MemfaultMinimalCbor, Test_All) {
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
