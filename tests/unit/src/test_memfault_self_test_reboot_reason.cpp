#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "components/core/src/memfault_self_test_private.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/reboot_tracking.h"

extern "C" {
MEMFAULT_NORETURN void memfault_platform_reboot(void) {
  mock().actualCall(__func__);
}

void memfault_reboot_tracking_mark_reset_imminent(eMemfaultRebootReason reboot_reason,
                                                  const sMfltRebootTrackingRegInfo *reg) {
  mock()
    .actualCall(__func__)
    .withParameter("reboot_reason", reboot_reason)
    .withParameter("reg", (const void *)reg);
}
}

TEST_GROUP(MemfaultSelfTestRebootReason) {
  void setup() { }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestRebootReason, Test_RebootTest) {
  // Call reboot test function and check that mocks are called and set correctly
  mock()
    .expectOneCall("memfault_reboot_tracking_mark_reset_imminent")
    .withParameter("reboot_reason", kMfltRebootReason_SelfTest)
    .ignoreOtherParameters();
  mock().expectOneCall("memfault_platform_reboot");
  memfault_self_test_reboot_reason_test();
}

TEST(MemfaultSelfTestRebootReason, Test_RebootVerifyTest) {
  sMfltRebootReason reboot_reason = {
    .prior_stored_reason = kMfltRebootReason_SelfTest,
  };

  // Verify test fails if reboot reason is not set (reboot tracking not initialized)
  mock()
    .expectOneCall("memfault_reboot_tracking_get_reboot_reason")
    .withOutputParameterReturning("reboot_reason", &reboot_reason, sizeof(reboot_reason))
    .andReturnValue(1);
  unsigned int result = memfault_self_test_reboot_reason_test_verify();
  UNSIGNED_LONGS_EQUAL(1, result);

  // Verify test fails if reboot reason is not set to self test reason
  reboot_reason.prior_stored_reason = kMfltRebootReason_Unknown;
  mock()
    .expectOneCall("memfault_reboot_tracking_get_reboot_reason")
    .withOutputParameterReturning("reboot_reason", &reboot_reason, sizeof(reboot_reason))
    .andReturnValue(0);
  result = memfault_self_test_reboot_reason_test_verify();
  UNSIGNED_LONGS_EQUAL(1, result);

  // Verify test passes if reboot tracking initialized and correct reason returned
  reboot_reason.prior_stored_reason = kMfltRebootReason_SelfTest;
  mock()
    .expectOneCall("memfault_reboot_tracking_get_reboot_reason")
    .withOutputParameterReturning("reboot_reason", &reboot_reason, sizeof(reboot_reason))
    .andReturnValue(0);
  result = memfault_self_test_reboot_reason_test_verify();
  UNSIGNED_LONGS_EQUAL(0, result);
}
