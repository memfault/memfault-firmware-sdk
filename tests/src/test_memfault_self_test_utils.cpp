#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault_self_test_private.h"

TEST_GROUP(MemfaultSelfTestUtils) {
  void setup() {}
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestUtils, Test_ValidDeviceSerial) {
  char const *valid_device_serial =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
  // Iteratate through all ascii chars
  // Skip null as this would just terminate the string early
  for (unsigned int c = 1; c < 0x100; c++) {
    bool result = memfault_self_test_valid_device_serial((unsigned char)c);
    if (strchr(valid_device_serial, (char)c) != NULL) {
      CHECK(result);
    } else {
      CHECK_FALSE(result);
    }
  }
}

TEST(MemfaultSelfTestUtils, Test_ValidHwVersionSwType) {
  char const *valid_hw_version_sw_type =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_:.+";
  // Iteratate through all ascii chars
  // Skip null as this would just terminate the string early
  for (unsigned int c = 1; c < 0x100; c++) {
    bool result = memfault_self_test_valid_hw_version_sw_type((unsigned char)c);
    if (strchr(valid_hw_version_sw_type, (char)c) != NULL) {
      CHECK(result);
    } else {
      CHECK_FALSE(result);
    }
  }
}
