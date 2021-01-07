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
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_Assert, NULL);
  sMfltResetReasonInfo info;
  bool info_available = memfault_reboot_tracking_read_reset_info(&info);
  CHECK(!info_available);
  memfault_reboot_tracking_reset_crash_count();
  memfault_reboot_tracking_get_crash_count();
  memfault_reboot_tracking_clear_reset_info();
}

TEST(MfltStorageTestGroup, Test_CrashTracking) {
  const size_t num_loops = 3;
  for (size_t i = 0; i < num_loops; i++) {
    memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UnknownError, NULL);
    memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  }

  // shouldn't impact the crash count
  memfault_reboot_tracking_clear_reset_info();

  // mcu reset reason being appended without a pre-existing reset reason should bump crash count
  sResetBootupInfo bootup_info = {
    .reset_reason_reg = 0x1,
    .reset_reason = kMfltRebootReason_HardFault,
  };
  memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region, &bootup_info);

  size_t crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(num_loops + 1, crash_count);

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

  // The POR and Firmware Update should not have been counted as a crash
  crash_count = memfault_reboot_tracking_get_crash_count();
  LONGS_EQUAL(3, crash_count);
}
