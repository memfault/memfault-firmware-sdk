#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <string.h>

#include "memfault/panics/assert.h"


static sMemfaultAssertInfo p_assert_info = {0};

void memfault_fault_handling_assert_extra(void *pc, void *lr, sMemfaultAssertInfo *extra_info) {
  (void)pc, (void)lr;
  LONGS_EQUAL(p_assert_info.extra, extra_info->extra);
  LONGS_EQUAL(p_assert_info.assert_reason, extra_info->assert_reason);

  mock().actualCall(__func__);
}

void memfault_fault_handling_assert(void *pc, void *lr) {
  (void)pc, (void)lr;
  mock().actualCall(__func__);
}

TEST_GROUP(Assert) {
  void setup() {
    mock().strictOrder();
    memset(&p_assert_info, 0, sizeof(p_assert_info));
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(Assert, Test_AssertMacrosOK) {
  mock().expectOneCall("memfault_fault_handling_assert_extra");
  p_assert_info.extra = 0x12345678;
  p_assert_info.assert_reason = kMfltRebootReason_BusFault;
  MEMFAULT_ASSERT_EXTRA_AND_REASON(p_assert_info.extra, kMfltRebootReason_BusFault);

  mock().expectOneCall("memfault_fault_handling_assert_extra");
  p_assert_info.extra++;
  p_assert_info.assert_reason = kMfltRebootReason_Assert;
  MEMFAULT_ASSERT_RECORD(p_assert_info.extra);

  mock().expectOneCall("memfault_fault_handling_assert_extra");
  p_assert_info.extra++;
  MEMFAULT_ASSERT_EXTRA(0, p_assert_info.extra);

  mock().expectOneCall("memfault_fault_handling_assert_extra");
  p_assert_info.extra = 0;
  p_assert_info.assert_reason = kMfltRebootReason_SoftwareWatchdog;
  MEMFAULT_SOFTWARE_WATCHDOG();

  mock().expectOneCall("memfault_fault_handling_assert");
  p_assert_info.extra++;
  MEMFAULT_ASSERT(0);
}
