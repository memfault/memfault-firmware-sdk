#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/reboot_tracking.h"

extern "C" {
void memfault_reboot_tracking_mark_reset_imminent(eMemfaultRebootReason reboot_reason,
                                                  const sMfltRebootTrackingRegInfo *reg) {
  mock()
    .actualCall(__func__)
    .withParameter("reboot_reason", reboot_reason)
    .withParameter("reg", (const void *)reg);
}
}

TEST_GROUP(MfltUserRebootReasons) {
  void setup() { }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltUserRebootReasons, Test_ExpectedUserRebootReasons) {
  uint16_t expected_base = MEMFAULT_REBOOT_REASON_EXPECTED_CUSTOM_BASE;
  LONGS_EQUAL(++expected_base, MEMFAULT_REBOOT_REASON_KEY(ExpectedReboot1));
  LONGS_EQUAL(++expected_base, MEMFAULT_REBOOT_REASON_KEY(ExpectedReboot2));
  LONGS_EQUAL(++expected_base, MEMFAULT_REBOOT_REASON_KEY(ExpectedReboot3));
}

TEST(MfltUserRebootReasons, Test_UnexpectedUserRebootReasons) {
  uint16_t unexpected_base = MEMFAULT_REBOOT_REASON_UNEXPECTED_CUSTOM_BASE;
  LONGS_EQUAL(++unexpected_base, MEMFAULT_REBOOT_REASON_KEY(UnexpectedReboot1));
  LONGS_EQUAL(++unexpected_base, MEMFAULT_REBOOT_REASON_KEY(UnexpectedReboot2));
  LONGS_EQUAL(++unexpected_base, MEMFAULT_REBOOT_REASON_KEY(UnexpectedReboot3));
}

TEST(MfltUserRebootReasons, Test_MarkResetWrapper) {
  mock()
    .expectOneCall("memfault_reboot_tracking_mark_reset_imminent")
    .withParameter("reboot_reason", MEMFAULT_REBOOT_REASON_KEY(UnexpectedReboot1))
    .ignoreOtherParameters();

  MEMFAULT_REBOOT_MARK_RESET_IMMINENT_CUSTOM(UnexpectedReboot1);
}
