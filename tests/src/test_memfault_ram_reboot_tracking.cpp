#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "memfault/core/reboot_tracking.h"
#include "memfault_reboot_tracking_private.h"

static uint8_t s_mflt_reboot_tracking_region[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];
}

static void prv_check_crash_count(size_t expected_crash_count) {
  size_t actual_crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(expected_crash_count, actual_crash_count);
}

static void prv_check_reboot_info(sMfltResetReasonInfo *expected_reboot_info) {
  sMfltResetReasonInfo actual_info;
  bool result = memfault_reboot_tracking_read_reset_info(&actual_info);

  CHECK(result);
  LONGS_EQUAL(expected_reboot_info->reason, actual_info.reason);
  LONGS_EQUAL(expected_reboot_info->pc, actual_info.pc);
  LONGS_EQUAL(expected_reboot_info->lr, actual_info.lr);
  LONGS_EQUAL(expected_reboot_info->reset_reason_reg0, actual_info.reset_reason_reg0);
  LONGS_EQUAL(expected_reboot_info->coredump_saved, actual_info.coredump_saved);
}

static void prv_check_reboot_reason_read(sMfltRebootReason *expected_reboot_reason) {
  sMfltRebootReason actual_reboot_reason;
  int result = memfault_reboot_tracking_get_reboot_reason(&actual_reboot_reason);
  LONGS_EQUAL(result, 0);
  LONGS_EQUAL(expected_reboot_reason->prior_stored_reason,
              actual_reboot_reason.prior_stored_reason);
  LONGS_EQUAL(expected_reboot_reason->reboot_reg_reason, actual_reboot_reason.reboot_reg_reason);
}

static void prv_check_unexpected_reboot_occurred(bool expected_value) {
  bool actual_value;
  int result = memfault_reboot_tracking_get_unexpected_reboot_occurred(&actual_value);
  LONGS_EQUAL(result, 0);
  LONGS_EQUAL(expected_value, actual_value);
}

TEST_GROUP(MfltRamRebootTracking) {
  void setup() {
    // simulate memory initializing with random pattern at boot
    memset(&s_mflt_reboot_tracking_region[0], 0xBA, sizeof(s_mflt_reboot_tracking_region));
    memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);
    memfault_reboot_tracking_reset_crash_count();
    memfault_reboot_tracking_clear_reboot_reason();
    memfault_reboot_tracking_clear_reset_info();
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltRamRebootTracking, Test_ReadResetInfo_BadArgs) {
  // Check returns false with NULL param
  CHECK_FALSE(memfault_reboot_tracking_read_reset_info(NULL));
}

TEST(MfltRamRebootTracking, Test_ReadResetInfo_AfterReset) {
  // Reset info already cleared in setup() above
  // Check returns false when reset info is cleared/reset
  sMfltResetReasonInfo info;
  CHECK_FALSE(memfault_reboot_tracking_read_reset_info(&info));
}

TEST(MfltRamRebootTracking, Test_NoOpsWithBadRegion) {
  // Mark coredump, boot with crash
  memfault_reboot_tracking_mark_coredump_saved();

  sResetBootupInfo info = { .reset_reason = kMfltRebootReason_BusFault };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);

  // This point, we could expected coredump_saved = true, crash_count = 1, etc
  // Instead boot with a NULL region, this will clear the internal pointer and result in failed
  // operations
  memfault_reboot_tracking_boot(NULL, NULL);

  // Check crash count is not reset in buffer
  sMfltRebootInfo *reboot_info = (sMfltRebootInfo *)s_mflt_reboot_tracking_region;

  size_t crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);
  LONGS_EQUAL(1, reboot_info->crash_count);

  // Check reset crash does not affect buffer
  memfault_reboot_tracking_reset_crash_count();
  LONGS_EQUAL(1, reboot_info->crash_count);

  // Check clear reset info does not clear
  memfault_reboot_tracking_clear_reset_info();
  CHECK(reboot_info->coredump_saved);

  // Check coredump marked returns false
  reboot_info->coredump_saved = false;
  memfault_reboot_tracking_mark_coredump_saved();
  CHECK_FALSE(reboot_info->coredump_saved);

  // Check that mark reset imminent does modify the reason
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);
  LONGS_EQUAL(reboot_info->last_reboot_reason, kMfltRebootReason_BusFault);
}

TEST(MfltRamRebootTracking, Test_ReadResetInfo) {
  sMfltResetReasonInfo read_info;
  bool crash_found = memfault_reboot_tracking_read_reset_info(&read_info);
  CHECK(!crash_found);

  sMfltRebootTrackingRegInfo info = { 0 };
  info.pc = 0x1;
  info.lr = 0x2;

  const eMemfaultRebootReason reason = kMfltRebootReason_Assert;
  memfault_reboot_tracking_mark_reset_imminent(reason, &info);
  memfault_reboot_tracking_mark_coredump_saved();

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_info = (sMfltResetReasonInfo){
    .reason = reason,
    .pc = info.pc,
    .lr = info.lr,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
    .coredump_saved = true,
  };
  prv_check_reboot_info(&expected_info);
}

TEST(MfltRamRebootTracking, Test_NoRamRegionInitialized) {
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);
  memfault_reboot_tracking_boot(NULL, NULL);

  sMfltResetReasonInfo info;
  bool info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
}

TEST(MfltRamRebootTracking, Test_CrashTracking) {
  // Mark next bootup as created by an error
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UnknownError, NULL);

  // Crash count should not be incremented yet
  prv_check_crash_count(0);
  memfault_reboot_tracking_clear_reset_info();

  // Boot with reboot reason, no previously set reason, should bump crash count
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
    .reset_reason = kMfltRebootReason_HardFault,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  prv_check_crash_count(1);

  // Resetting reboot tracking should not alter crash count
  memfault_reboot_tracking_clear_reset_info();
  prv_check_crash_count(1);

  memfault_reboot_tracking_reset_crash_count();
  prv_check_crash_count(0);
}

TEST(MfltRamRebootTracking, Test_GetRebootReason_BadArgs) {
  // Test that reason is invalid if arg is NULL
  int rc = memfault_reboot_tracking_get_reboot_reason(NULL);

  LONGS_EQUAL(-1, rc);

  // Test that reason is invalid before reboot tracking has booted
  // Set a non-zero value in a field to check that field was not modified
  sMfltRebootReason reboot_reason = {
    .reboot_reg_reason = kMfltRebootReason_UnknownError,
  };
  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(-1, rc);
  LONGS_EQUAL((long)kMfltRebootReason_UnknownError, (long)reboot_reason.reboot_reg_reason);
}

TEST(MfltRamRebootTracking, Test_GetRebootReason_BootNull) {
  // Test booting with null
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_Unknown,
    .prior_stored_reason = kMfltRebootReason_Unknown,
  };
  prv_check_reboot_reason_read(&expected_reboot_reason);
}

TEST(MfltRamRebootTracking, Test_GetRebootReason_BootupReason) {
  // Test booting with a non-error reason
  sResetBootupInfo info = {
    .reset_reason = kMfltRebootReason_SoftwareReset,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_SoftwareReset,
    .prior_stored_reason = kMfltRebootReason_SoftwareReset,
  };
  prv_check_reboot_reason_read(&expected_reboot_reason);

  // Check that reboot reason is not cleared by
  // memfault_reboot_tracking_clear_reset_info
  memfault_reboot_tracking_clear_reset_info();
  prv_check_reboot_reason_read(&expected_reboot_reason);
}

TEST(MfltRamRebootTracking, Test_GetRebootReason_MarkedReason) {
  // Simulate a crash, mark the reason. This will store the crash reason as prior_stored_reason
  // and reboot_reg_reason with bootup reason
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);

  sResetBootupInfo info = (sResetBootupInfo){
    .reset_reason = kMfltRebootReason_PinReset,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_PinReset,
    .prior_stored_reason = kMfltRebootReason_Assert,
  };
  prv_check_reboot_reason_read(&expected_reboot_reason);
}

TEST(MfltRamRebootTracking, Test_GetUnexpectedRebootOccurred_BadArgs) {
  // Test invalid if arg is NULL
  int rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(NULL);
  LONGS_EQUAL(-1, rc);

  bool unexpected_reboot;
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(-1, rc);
}

TEST(MfltRamRebootTracking, Test_GetUnexpectedRebootOccurred_Expected) {
  // Test booting with an expected reason
  sResetBootupInfo info = { .reset_reason = kMfltRebootReason_SoftwareReset };

  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  prv_check_unexpected_reboot_occurred(false);
}

TEST(MfltRamRebootTracking, Test_GetUnexpectedRebootOccurred_Unexpected) {
  // Test booting with an unexpected reason
  sResetBootupInfo info = { .reset_reason = kMfltRebootReason_UnknownError };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  prv_check_unexpected_reboot_occurred(true);
}

TEST(MfltRamRebootTracking, Test_RebootTrackingBooted) {
  // Reset the struct back to uninitialized and module pointer to null
  memset(&s_mflt_reboot_tracking_region[0], 0xBA, sizeof(s_mflt_reboot_tracking_region));
  memfault_reboot_tracking_boot(NULL, NULL);

  // Check that reboot tracking not booted
  CHECK_FALSE(memfault_reboot_tracking_booted());

  // Check that reboot tracking booted
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);
  CHECK(memfault_reboot_tracking_booted());
}

// Reboot Scenario Test Cases
//
// There are several scenarios we need to handle with reboot tracking
// 1. The device only has the boot reason (expected/unexpected), no reason marked before reboot
// 2. The device has both a marked reason (expected/unexpected), and a boot reason
// (expected/unexpected)
// 3. The device has only a marked reason (expected/unexpected) before reboot, no reason at boot
// 4. The device has no marked reason and no reason at boot
//
// Each case below we verify the following functions return values matching the scenario:
// - memfault_reboot_tracking_get_crash_count()
// - memfault_reboot_tracking_read_reset_info()
// - memfault_reboot_tracking_get_reboot_reason()
// - memfault_reboot_tracking_get_unexpected_reboot_occurred()

// Boot Reason Tests

TEST(MfltRamRebootTracking, Test_BootExpectedReason) {
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_DeepSleep,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = bootup_info.reset_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = bootup_info.reset_reason,
  };
  prv_check_crash_count(0);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(false);
}

TEST(MfltRamRebootTracking, Test_BootUnexpectedReason) {
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_Assert,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = bootup_info.reset_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = bootup_info.reset_reason,
  };
  prv_check_crash_count(1);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(true);
}

// Marked Reason + Boot Reason Tests

TEST(MfltRamRebootTracking, Test_MarkedExpectedBootUnexpectedReason) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_FirmwareUpdate;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_Unknown,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(0);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(false);
}

TEST(MfltRamRebootTracking, Test_MarkedExpectedBootExpectedReason) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_FirmwareUpdate;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_UserReset,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(0);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(false);
}

TEST(MfltRamRebootTracking, Test_MarkedUnexpectedBootExpectedReason) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_HardFault;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_ButtonReset,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(1);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(true);
}

TEST(MfltRamRebootTracking, Test_MarkedUnexpectedBootUnexpectedReason) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_HardFault;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_BusFault,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
    .reset_reason_reg0 = bootup_info.reset_reason_reg,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = bootup_info.reset_reason,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(1);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(true);
}

// Marked Reason + No Boot Reason Tests

TEST(MfltRamRebootTracking, Test_MarkedExpectedReasonBootNull) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_FirmwareUpdate;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_Unknown,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(0);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(false);
}

TEST(MfltRamRebootTracking, Test_MarkedUnexpectedReasonBootNull) {
  eMemfaultRebootReason marked_reason = kMfltRebootReason_Assert;
  memfault_reboot_tracking_mark_reset_imminent(marked_reason, NULL);

  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = marked_reason,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_Unknown,
    .prior_stored_reason = marked_reason,
  };
  prv_check_crash_count(1);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(true);
}

// No Marked Reason + No Boot Reason Test

TEST(MfltRamRebootTracking, Test_BootNull) {
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  sMfltResetReasonInfo expected_reboot_info = (sMfltResetReasonInfo){
    .reason = kMfltRebootReason_Unknown,
  };

  sMfltRebootReason expected_reboot_reason = (sMfltRebootReason){
    .reboot_reg_reason = kMfltRebootReason_Unknown,
    .prior_stored_reason = kMfltRebootReason_Unknown,
  };
  prv_check_crash_count(1);
  prv_check_reboot_info(&expected_reboot_info);
  prv_check_reboot_reason_read(&expected_reboot_reason);
  prv_check_unexpected_reboot_occurred(true);
}
