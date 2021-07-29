//! @file
//!

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stdlib.h>

#include "memfault_test_compact_log_c.h"

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
