//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "memfault/components.h"

TEST_GROUP(MemfaultNrf5Port){
  void setup() {
  }
  void teardown() {
  }
};

TEST(MemfaultNrf5Port, Test_RunsOk) {
  size_t bound = memfault_platform_sanitize_address_range((void *)0x20000000, 0x1000);
  LONGS_EQUAL(0x1000, bound);

  bound = memfault_platform_sanitize_address_range((void *)0x20000000, 0x20000000);
  LONGS_EQUAL(0x40000, bound);

  bound = memfault_platform_sanitize_address_range((void *)0, 1);
  LONGS_EQUAL(0, bound);
}
