//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Note: These tests are in a C (instead of CPP) file because they
//! test the use of the _Generic() macro which is not available in CPP.

#include "memfault_test_compact_log_c.h"

#include <stdint.h>
#include <inttypes.h>

#include "memfault/core/compact_log_helpers.h"
#include "memfault/core/compact_log_compile_time_checks.h"

#include "CppUTestExt/MockSupport_c.h"

static void prv_compact_log_mock(uint32_t compressed_fmt, ...) {
  mock_c()->actualCall(__func__)->withUnsignedIntParameters("compressed_fmt", compressed_fmt);
}

static void prv_expect_compact_log_call(uint32_t compressed_fmt) {
  mock_c()->expectOneCall("prv_compact_log_mock")->withUnsignedIntParameters(
      "compressed_fmt", compressed_fmt);
}

#define MOCK_CHECK_EXPECTIONS_AND_CLEAR() \
do { \
  mock_c()->checkExpectations(); \
  mock_c()->clear(); \
} while (0)

#include "test_memfault_compact_log_cxx.c"

void test_compact_log_c_argument_expansion(void) {
  test_compact_log_cxx_argument_expansion();
}

void test_compact_log_c_multi_arg(void) {
  test_compact_log_cxx_multi_arg();
}

void test_compact_log_c_multi_complex(void) {
  test_compact_log_cxx_multi_complex();
}
