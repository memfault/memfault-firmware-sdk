#include "CppUTest/TestHarness.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/reboot_tracking.h"

TEST_GROUP(MfltUserRebootReasons) {
  void setup() { }

  void teardown() { }
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
