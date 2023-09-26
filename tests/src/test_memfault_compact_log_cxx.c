//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Note: These tests are reused in C++ and C tests. See the files including
//! this one for how they're meant to be used.

#include "memfault_test_compact_log_c.h"

#include <stdint.h>
#include <inttypes.h>

#include "memfault/core/compact_log_helpers.h"
#include "memfault/core/compact_log_compile_time_checks.h"

#include "CppUTestExt/MockSupport_c.h"

#define MEMFAULT_COMPACT_LOG_CHECK(format, ...)                       \
  do {                                                                \
    MEMFAULT_LOGGING_RUN_COMPILE_TIME_CHECKS(format, ## __VA_ARGS__); \
    prv_compact_log_mock(MFLT_GET_COMPRESSED_LOG_FMT(__VA_ARGS__),    \
                               ## __VA_ARGS__);                       \
  } while (0)

static void test_compact_log_cxx_argument_expansion(void) {
  // Up to 15 arguments should get serialized correctly
  // compressed format code for integers is 0b00
  for (int i = 0; i <= 15; i++) {
    const uint32_t compressed_fmt = 0x1 << (i *2);
    prv_expect_compact_log_call(compressed_fmt);
  }
  MEMFAULT_COMPACT_LOG_CHECK("0 args");
  MEMFAULT_COMPACT_LOG_CHECK("1 arg %d", 1);
  MEMFAULT_COMPACT_LOG_CHECK("2 arg %d %d", 1, 2);
  MEMFAULT_COMPACT_LOG_CHECK("3 arg %d %d %d", 1, 2, 3);
  MEMFAULT_COMPACT_LOG_CHECK("4 arg %d %d %d %d", 1, 2, 3, 4);
  MEMFAULT_COMPACT_LOG_CHECK("5 arg %d %d %d %d %d", 1, 2, 3, 4, 5);
  MEMFAULT_COMPACT_LOG_CHECK("6 arg %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6);
  MEMFAULT_COMPACT_LOG_CHECK("7 arg %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7);
  MEMFAULT_COMPACT_LOG_CHECK("8 arg %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8);
  MEMFAULT_COMPACT_LOG_CHECK("9 arg %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9);
  MEMFAULT_COMPACT_LOG_CHECK("10 arg %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  MEMFAULT_COMPACT_LOG_CHECK("11 arg %d %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
  MEMFAULT_COMPACT_LOG_CHECK("12 arg %d %d %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  MEMFAULT_COMPACT_LOG_CHECK("13 arg %d %d %d %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
  MEMFAULT_COMPACT_LOG_CHECK("14 arg %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
  MEMFAULT_COMPACT_LOG_CHECK("15 arg %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
}

static void test_compact_log_cxx_multi_arg(void) {
  MOCK_CHECK_EXPECTIONS_AND_CLEAR();
  
  prv_expect_compact_log_call(0x1C9);
  const uint8_t u8 = 8;
  const uint64_t u64 = 0x7fffeee10001234;

  MEMFAULT_COMPACT_LOG_CHECK("Assorted Types: %s %" PRIx8 " %f %" PRIx64,
                     "hello", u8, (double)2.1f, u64);
  MOCK_CHECK_EXPECTIONS_AND_CLEAR();

  const char *s1 = NULL;
  const char s2[] = "s2";
  const char *s3 = "s3";
  char s4[] = "s4";

  prv_expect_compact_log_call(0x1FF);
  MEMFAULT_COMPACT_LOG_CHECK("Strings %s %s %s %s", s1, s2, s3, s4);
}

static void test_compact_log_cxx_multi_complex(void) {
  // a variety of different argument types
  const char *s = "s";
  uint64_t llx = 0xdeadbeef1;
  double f = 1.2;
  int d = 10;
  int64_t lld = 1;

  // A complex formatter with all the different types interleaved
  // 0b111.0010.0101.1000.1110.0111.0001.1011
  prv_expect_compact_log_call(0x7258E71B);
  MEMFAULT_COMPACT_LOG_CHECK("%s %d %f %" PRIx64 ": "
                             "%" PRId64 " %f %d %s: "
                             "%f %" PRId64 " %s %d: "
                             "%" PRId64 " %f %s",
                             s, d, f, llx, lld, f, d, s, f, lld, s, d, lld, f, s);
}
