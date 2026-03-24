#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/math.h"
#include "memfault/core/sdk_assert.h"
#include "memfault/core/self_test.h"
#include "memfault_self_test_private.h"

TEST_GROUP(MemfaultSelfTestUtils) {
  void setup() { }
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

static void prv_arg_to_test_flag_helper(const char *arg, uint32_t expected_flags) {
  uint32_t actual_flags = memfault_self_test_arg_to_flag(arg);
  LONGS_EQUAL(expected_flags, actual_flags);
}

TEST(MemfaultSelfTestUtils, Test_ArgToTestFlag) {
  prv_arg_to_test_flag_helper("", kMemfaultSelfTestFlag_Default);
  prv_arg_to_test_flag_helper("bad_arg", kMemfaultSelfTestFlag_Default);
  prv_arg_to_test_flag_helper("reboot", kMemfaultSelfTestFlag_RebootReason);
  prv_arg_to_test_flag_helper("reboot_verify", kMemfaultSelfTestFlag_RebootReasonVerify);
}

static jmp_buf s_assert_jmp_buf;

void memfault_sdk_assert_func(void) {
  mock().actualCall(__func__);
  longjmp(s_assert_jmp_buf, -1);
}

TEST(MemfaultSelfTestUtils, Test_NullArg) {
  // We expect a call to the SDK assert
  mock().expectOneCall("memfault_sdk_assert_func");
  if (setjmp(s_assert_jmp_buf) == 0) {
    memfault_self_test_arg_to_flag(NULL);
  }
}

TEST(MemfaultSelfTestUtils, Test_Strnlen) {
  const char *test_str = "test_stri\0ng";
  const char test_no_null[5] = {
    'a', 'b', 'c', 'd', 'e',
  };

  // Test zero length
  size_t n = 0;
  size_t result = memfault_strnlen(test_str, n);
  LONGS_EQUAL(n, result);

  // Test length 1
  n = 1;
  result = memfault_strnlen(test_str, n);
  LONGS_EQUAL(n, result);

  // Test NULL at index >= n
  n = 9;
  result = memfault_strnlen(test_str, n);
  LONGS_EQUAL(n, result);

  // Test NULL at index < n
  n = 12;
  result = memfault_strnlen(test_str, n);
  LONGS_EQUAL(9, result);

  // Test no NULL in string
  n = MEMFAULT_ARRAY_SIZE(test_no_null);
  result = memfault_strnlen(test_no_null, n);
  LONGS_EQUAL(n, result);
}
