#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/self_test.h"
#include "memfault_self_test_private.h"

extern "C" {
uint32_t memfault_self_test_device_info_test(void) {
  return mock().actualCall(__func__).returnUnsignedIntValue();
}
uint32_t memfault_self_test_component_boot_test(void) {
  return mock().actualCall(__func__).returnUnsignedIntValue();
}
uint32_t memfault_self_test_coredump_regions_test(void) {
  return mock().actualCall(__func__).returnUnsignedIntValue();
}
void memfault_self_test_data_export_test(void) {
  mock().actualCall(__func__);
}

void memfault_self_test_reboot_reason_test(void) {
  mock().actualCall(__func__);
}

uint32_t memfault_self_test_reboot_reason_test_verify(void) {
  return mock().actualCall(__func__).returnUnsignedIntValue();
}
}

// Helper function for tests that return a result rather than just running
static void prv_run_single_component_test(const char *test_name, eMemfaultSelfTestFlag test_flag) {
  mock().expectOneCall(test_name).andReturnValue(0);
  int result = memfault_self_test_run(test_flag);
  LONGS_EQUAL(0, result);

  mock().expectOneCall(test_name).andReturnValue(1);
  result = memfault_self_test_run(test_flag);
  LONGS_EQUAL(1, result);
}

TEST_GROUP(MemfaultSelfTest) {
  void setup() { }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTest, Test_NoRunFlags) {
  int result = memfault_self_test_run(0);
  LONGS_EQUAL(0, result);
}

TEST(MemfaultSelfTest, Test_DeviceInfoTest) {
  prv_run_single_component_test("memfault_self_test_device_info_test",
                                kMemfaultSelfTestFlag_DeviceInfo);
}

TEST(MemfaultSelfTest, Test_ComponentBootTest) {
  prv_run_single_component_test("memfault_self_test_component_boot_test",
                                kMemfaultSelfTestFlag_ComponentBoot);
}

TEST(MemfaultSelfTest, Test_CoredumpRegionsTest) {
  prv_run_single_component_test("memfault_self_test_coredump_regions_test",
                                kMemfaultSelfTestFlag_CoredumpRegions);
}

TEST(MemfaultSelfTest, Test_DataExportTest) {
  mock().expectOneCall("memfault_self_test_data_export_test");
  memfault_self_test_run(kMemfaultSelfTestFlag_DataExport);
}

TEST(MemfaultSelfTest, Test_RebootReasonTest) {
  mock().expectOneCall("memfault_self_test_reboot_reason_test");
  memfault_self_test_run(kMemfaultSelfTestFlag_RebootReason);
}

TEST(MemfaultSelfTest, Test_RebootReasonTestVerify) {
  prv_run_single_component_test("memfault_self_test_reboot_reason_test_verify",
                                kMemfaultSelfTestFlag_RebootReasonVerify);
}

TEST(MemfaultSelfTest, Test_SelfTestDefaultHappyPath) {
  mock().expectOneCall("memfault_self_test_device_info_test").andReturnValue(0);
  mock().expectOneCall("memfault_self_test_component_boot_test").andReturnValue(0);
  mock().expectOneCall("memfault_self_test_coredump_regions_test").andReturnValue(0);
  mock().expectOneCall("memfault_self_test_data_export_test");

  int result = memfault_self_test_run(kMemfaultSelfTestFlag_Default);
  LONGS_EQUAL(0, result);
}

TEST(MemfaultSelfTest, Test_SelfTestDefaultComponentBootFail) {
  mock().expectOneCall("memfault_self_test_device_info_test").andReturnValue(0);
  mock().expectOneCall("memfault_self_test_component_boot_test").andReturnValue(1);
  mock().expectOneCall("memfault_self_test_coredump_regions_test").andReturnValue(0);
  mock().expectOneCall("memfault_self_test_data_export_test");

  int result = memfault_self_test_run(kMemfaultSelfTestFlag_Default);
  LONGS_EQUAL(1, result);
}

TEST(MemfaultSelfTest, Test_SelfTestRunFailure) {
  // Test with an unknown flag value, no mock calls expected
  memfault_self_test_run(0x1000);
}
