#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

extern "C" {
  #include "memfault/core/reboot_tracking.h"
  #include "memfault_reboot_tracking_private.h"

  static uint8_t s_mflt_reboot_tracking_region[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];
}

TEST_GROUP(MfltStorageTestGroup) {
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

TEST(MfltStorageTestGroup, Test_BadArgs) {
  memfault_reboot_tracking_read_reset_info(NULL);
}

TEST(MfltStorageTestGroup, Test_SetAndGetCrashInfo) {
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
  CHECK(memfault_reboot_tracking_booted());

  crash_found = memfault_reboot_tracking_read_reset_info(&read_info);
  CHECK(crash_found);

  LONGS_EQUAL(reason, read_info.reason);
  LONGS_EQUAL(info.pc, read_info.pc);
  LONGS_EQUAL(info.lr, read_info.lr);
  LONGS_EQUAL(bootup_info.reset_reason_reg, read_info.reset_reason_reg0);
  LONGS_EQUAL(1, read_info.coredump_saved);

  memfault_reboot_tracking_clear_reset_info();
  crash_found = memfault_reboot_tracking_read_reset_info(&read_info);
  CHECK(!crash_found);
}

TEST(MfltStorageTestGroup, Test_NoRamRegionIntialized) {
  memfault_reboot_tracking_boot(NULL, NULL);
  CHECK_FALSE(memfault_reboot_tracking_booted());
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);
  sMfltResetReasonInfo info;
  bool info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
  memfault_reboot_tracking_reset_crash_count();
  memfault_reboot_tracking_get_crash_count();
  memfault_reboot_tracking_clear_reset_info();
}

TEST(MfltStorageTestGroup, Test_CrashTracking) {
  // Mark next bootup as created by an error
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UnknownError, NULL);

  // Crash count should not be incremented yet
  size_t crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);

  // mcu reset reason being appended without a pre-existing reset reason should bump crash count
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
    .reset_reason = kMfltRebootReason_HardFault,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  // Resetting reboot tracking should not alter crash count
  memfault_reboot_tracking_clear_reset_info();
  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  memfault_reboot_tracking_reset_crash_count();
  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);
}

TEST(MfltStorageTestGroup, Test_RebootSequence) {
  size_t crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(0, crash_count);
  // 1. Watchdog Reboot - Confirm we get a Watchdog and then no more info
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_SoftwareWatchdog, NULL);
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(1, crash_count);

  sMfltResetReasonInfo info;
  bool info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kMfltRebootReason_SoftwareWatchdog, info.reason);
  memfault_reboot_tracking_clear_reset_info();
  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
  // 2. Unexpected Reboot - Info should be taken from what we provide on bootup info
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0xab,
    .reset_reason = kMfltRebootReason_Assert,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(2, crash_count);
  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(bootup_info.reset_reason, info.reason);
  LONGS_EQUAL(bootup_info.reset_reason_reg, info.reset_reason_reg0);
  memfault_reboot_tracking_clear_reset_info();

  // 2 Firmware Update Reboot reboot - Reset region reg should be amended to info
  // but reset reason should not overwrite value
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_FirmwareUpdate, NULL);
  bootup_info.reset_reason_reg = 0xdead;

  // The kMfltRebootReason_Assert passed as part of bootup_info should be counted as a crash
  // but the reset reason should be the reboot which first kicked off the sequence of events
  // (kMfltRebootReason_FirmwareUpdate)
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);
  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kMfltRebootReason_FirmwareUpdate, info.reason);
  LONGS_EQUAL(bootup_info.reset_reason_reg, info.reset_reason_reg0);
  memfault_reboot_tracking_clear_reset_info();

  // 4. Unexpected Reboot (i.e POR) - Should see unknown as reset reason
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);
  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kMfltRebootReason_Unknown, info.reason);
  memfault_reboot_tracking_clear_reset_info();

  // 5. Expected reboot due to firmware update, boot with an expected reboot reason
  bootup_info.reset_reason = kMfltRebootReason_SoftwareReset;
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_FirmwareUpdate, NULL);
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kMfltRebootReason_FirmwareUpdate, info.reason);
  memfault_reboot_tracking_clear_reset_info();

  // 6. Boot with an expected reboot reason
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);
  info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(info_available);
  LONGS_EQUAL(kMfltRebootReason_SoftwareReset, info.reason);

  // Scenarios 5 and 6 should not count as unexpected crashes
  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(4, crash_count);
}

TEST(MfltStorageTestGroup, Test_GetRebootReason) {
  sMfltRebootReason reboot_reason = {
    .reboot_reg_reason = kMfltRebootReason_UnknownError,
  };
  int rc = 0;

  // Test that reason is invalid if arg is NULL
  rc = memfault_reboot_tracking_get_reboot_reason(NULL);

  LONGS_EQUAL(-1, rc);

  // Test that reason is invalid before reboot tracking has booted
  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(-1, rc);
  LONGS_EQUAL((long)kMfltRebootReason_UnknownError, (long)reboot_reason.reboot_reg_reason);

  // Test booting with null
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);

  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(0, rc);
  LONGS_EQUAL((long)kMfltRebootReason_Unknown, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kMfltRebootReason_Unknown, (long)reboot_reason.prior_stored_reason);

  // Test booting with a non-error reason, check that reboot reason is not cleared by
  // memfault_reboot_tracking_clear_reset_info
  memfault_reboot_tracking_clear_reset_info();
  memfault_reboot_tracking_clear_reboot_reason();

  sResetBootupInfo info = {
    .reset_reason = kMfltRebootReason_SoftwareReset,
  };

  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kMfltRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kMfltRebootReason_SoftwareReset, (long)reboot_reason.prior_stored_reason);

  memfault_reboot_tracking_clear_reset_info();
  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kMfltRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kMfltRebootReason_SoftwareReset, (long)reboot_reason.prior_stored_reason);

  // Simulate a crash, first boot with a crash. This will store the crash reason in reboot tracking
  // and reboot reason memory. Check values
  // Next clear reboot reason memory, and boot again with a different reason. Check values
  info.reset_reason = kMfltRebootReason_Assert;
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kMfltRebootReason_Assert, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kMfltRebootReason_Assert, (long)reboot_reason.prior_stored_reason);

  memfault_reboot_tracking_clear_reboot_reason();
  info.reset_reason = kMfltRebootReason_SoftwareReset;
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_reboot_reason(&reboot_reason);

  LONGS_EQUAL(rc, 0);
  LONGS_EQUAL((long)kMfltRebootReason_SoftwareReset, (long)reboot_reason.reboot_reg_reason);
  LONGS_EQUAL((long)kMfltRebootReason_Assert, (long)reboot_reason.prior_stored_reason);
}

TEST(MfltStorageTestGroup, Test_GetUnexpectedRebootOccurred) {
  bool unexpected_reboot = false;
  int rc = 0;
  sResetBootupInfo info = {.reset_reason = kMfltRebootReason_SoftwareReset};

  // Test invalid if arg is NULL
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(NULL);
  LONGS_EQUAL(-1, rc);

  // Test invalid before reboot tracking has booted
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(-1, rc);

  // Test booting with an expected reason
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK_FALSE(unexpected_reboot);
  memfault_reboot_tracking_clear_reset_info();

  // Test booting with an unexpected reason
  info.reset_reason = kMfltRebootReason_UnknownError;
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK(unexpected_reboot);
  memfault_reboot_tracking_clear_reset_info();

  // Test booting with an unexpected reason marked, but expected reason at boot
  info.reset_reason = kMfltRebootReason_PowerOnReset;
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &info);
  rc = memfault_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  LONGS_EQUAL(0, rc);
  CHECK(unexpected_reboot);
}

TEST(MfltStorageTestGroup, Test_RebootTrackingBooted) {
  // Reset the struct back to uninitialized and module pointer to null
  memset(&s_mflt_reboot_tracking_region[0], 0xBA, sizeof(s_mflt_reboot_tracking_region));
  memfault_reboot_tracking_boot(NULL, NULL);

  CHECK_FALSE(memfault_reboot_tracking_booted());
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, NULL);
  CHECK(memfault_reboot_tracking_booted());
}
