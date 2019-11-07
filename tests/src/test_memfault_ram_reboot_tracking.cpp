#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"

#include <string.h>

extern "C" {
  #include "memfault/panics/reboot_tracking.h"
  #include "memfault_reboot_tracking_private.h"

  static uint8_t s_mflt_reboot_tracking_region[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];
  #define MEMFAULT_NUM_REBOOTS_UNSTABLE 3
}

TEST_GROUP(MfltStorageTestGroup) {
  void setup() {
    memfault_reboot_tracking_boot(s_mflt_reboot_tracking_region);
  }

  void teardown() {
    memset(&s_mflt_reboot_tracking_region[0], 0x00, sizeof(s_mflt_reboot_tracking_region));
  }
};

TEST(MfltStorageTestGroup, Test_MfltAppNotLaunching) {
  bool unstable;
  for (int i = 0; i < MEMFAULT_NUM_REBOOTS_UNSTABLE; i++) {
    unstable = memfault_reboot_tracking_is_firmware_unstable();
    CHECK(!unstable);

    memfault_reboot_tracking_mark_app_launch_attempted();
  }

  unstable = memfault_reboot_tracking_is_firmware_unstable();
  CHECK(unstable);

  memfault_reboot_tracking_mark_system_started();
  unstable = memfault_reboot_tracking_is_firmware_unstable();
  CHECK(!unstable);
}

TEST(MfltStorageTestGroup, Test_UnstableMainApp) {
  sMfltCrashInfo info = {
    .reason = kMfltRebootReason_Assert,
  };

  bool unstable;
  for (int i = 0; i < MEMFAULT_NUM_REBOOTS_UNSTABLE; i++) {
    unstable = memfault_reboot_tracking_is_firmware_unstable();
    CHECK(!unstable);

    memfault_reboot_tracking_mark_crash(&info);
  }

  unstable = memfault_reboot_tracking_is_firmware_unstable();
  CHECK(unstable);

  memfault_reboot_tracking_mark_system_stable();
  unstable = memfault_reboot_tracking_is_firmware_unstable();
  CHECK(!unstable);
}

TEST(MfltStorageTestGroup, Test_SetAndGetCrashInfo) {
  sMfltCrashInfo info;
  bool crash_found = memfault_reboot_tracking_get_crash_info(&info);
  CHECK(!crash_found);

  info.reason = kMfltRebootReason_Assert;
  info.pc = 0x1;
  info.lr = 0x2;

  memfault_reboot_tracking_mark_crash(&info);

  sMfltCrashInfo read_info;
  crash_found = memfault_reboot_tracking_get_crash_info(&read_info);
  CHECK(crash_found);

  LONGS_EQUAL(info.reason, read_info.reason);
  LONGS_EQUAL(info.pc, read_info.pc);
  LONGS_EQUAL(info.lr, read_info.lr);

  memfault_reboot_tracking_clear_crash_info();
  crash_found = memfault_reboot_tracking_get_crash_info(&info);
  CHECK(!crash_found);
}

TEST(MfltStorageTestGroup, Test_UninitRegion) {
  memset(&s_mflt_reboot_tracking_region[0], 0xBA, sizeof(s_mflt_reboot_tracking_region));
  memfault_reboot_tracking_mark_system_started();
}
