#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <setjmp.h>
#include <stddef.h>
#include <string.h>

static jmp_buf s_assert_jmp_buf;

#include "memfault/core/platform/core.h"
#include "memfault/core/sdk_assert.h"

void memfault_platform_halt_if_debugging(void) {
  mock().actualCall(__func__);
}

void memfault_sdk_assert_func_noreturn(void) {
  // we make use of longjmp because this is a noreturn function
  longjmp(s_assert_jmp_buf, -1);
}

TEST_GROUP(MfltSdkAssert) {
  void setup() { }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltSdkAssert, Test_MfltCircularBufferInit) {
  mock().expectOneCall("memfault_platform_halt_if_debugging");
  if (setjmp(s_assert_jmp_buf) == 0) {
    memfault_sdk_assert_func();
  }
}
