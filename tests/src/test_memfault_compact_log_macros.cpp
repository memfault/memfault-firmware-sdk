//! @file
//!

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stdlib.h>

#include "memfault_test_compact_log_c.h"

#include "memfault/core/compact_log_helpers.h"
#include "memfault/core/compact_log_compile_time_checks.h"

TEST_GROUP(MfltCompactLogMacros) {
  void setup() {
    mock().strictOrder();
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltCompactLogMacros, Test_MfltCompactLog_ArgExpansion) {
  test_compact_log_c_argument_expansion();
}

TEST(MfltCompactLogMacros, Test_MfltCompactLog_MultiArg) {
  test_compact_log_c_multi_arg();
}

TEST(MfltCompactLogMacros, Test_MfltCompactLog_Complex) {
  test_compact_log_c_multi_complex();
}

//! Mocks and helper for the Cxx->C++ generic tests below
static void prv_compact_log_mock(uint32_t compressed_fmt, ...) {
  mock().actualCall(__func__).withUnsignedIntParameter("compressed_fmt", compressed_fmt);
}

static void prv_expect_compact_log_call(uint32_t compressed_fmt) {
  mock().expectOneCall("prv_compact_log_mock").withUnsignedIntParameter(
      "compressed_fmt", compressed_fmt);
}
#define MOCK_CHECK_EXPECTIONS_AND_CLEAR() \
do { \
  mock().checkExpectations(); \
  mock().clear(); \
} while (0)

#include "test_memfault_compact_log_cxx.c"

TEST(MfltCompactLogMacros, Test_MfltCompactLog_Cpp_ArgExpansion) {
  test_compact_log_cxx_argument_expansion();
}

TEST(MfltCompactLogMacros, Test_MfltCompactLog_Cpp_MultiArg) {
  test_compact_log_cxx_multi_arg();
}

TEST(MfltCompactLogMacros, Test_MfltCompactLog_Cpp_Complex) {
  test_compact_log_cxx_multi_complex();
}
