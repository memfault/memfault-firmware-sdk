#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/log.h"
#include "memfault/core/math.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/core/trace_event.h"
#include "memfault_self_test_private.h"
#include "mocks/mock_memfault_platform_debug_log.h"

extern "C" {

bool memfault_event_storage_booted(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_log_booted(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_reboot_tracking_booted(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_trace_event_booted(void) {
  return mock().actualCall(__func__).returnBoolValue();
}
}

TEST_GROUP(MemfaultSelfTestComponentBooted) {
  void setup() {}
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestComponentBooted, Test_ComponentBootNoneBooted) {
  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component       | Booted?|",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  const char *error_output_lines[] = {
    "Event Storage   |      no|",
    "Logging         |      no|",
    "Reboot Tracking |      no|",
    "Trace Event     |      no|",
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_output_lines,
                                 MEMFAULT_ARRAY_SIZE(error_output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(false);
  mock().expectOneCall("memfault_log_booted").andReturnValue(false);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(false);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(false);
  uint32_t result = memfault_self_test_component_boot_test();
  UNSIGNED_LONGS_EQUAL(0xf, result);
}

TEST(MemfaultSelfTestComponentBooted, Test_ComponentBootSingleBoot) {
  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component       | Booted?|",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  const char *error_output_lines[] = {
    "Event Storage   |      no|",
    "Logging         |      no|",
    "Trace Event     |      no|",
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_output_lines,
                                 MEMFAULT_ARRAY_SIZE(error_output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(false);
  mock().expectOneCall("memfault_log_booted").andReturnValue(false);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(true);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(false);
  uint32_t result = memfault_self_test_component_boot_test();
  UNSIGNED_LONGS_EQUAL(0xb, result);
}

TEST(MemfaultSelfTestComponentBooted, Test_ComponentBootAllBooted) {
  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Component       | Booted?|",
    "-----------------------------",
    "All components booted",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(true);
  mock().expectOneCall("memfault_log_booted").andReturnValue(true);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(true);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(true);
  uint32_t result = memfault_self_test_component_boot_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}
