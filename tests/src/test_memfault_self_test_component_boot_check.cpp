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
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component       | Booted?|",
    "-----------------------------", "Event Storage   |      no|",
    "Logging         |      no|",    "Reboot Tracking |      no|",
    "Trace Event     |      no|",    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(false);
  mock().expectOneCall("memfault_log_booted").andReturnValue(false);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(false);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(false);
  memfault_self_test_component_boot_test();
}

TEST(MemfaultSelfTestComponentBooted, Test_ComponentBootSingleBoot) {
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component       | Booted?|",
    "-----------------------------", "Event Storage   |      no|",
    "Logging         |      no|",    "Reboot Tracking |     yes|",
    "Trace Event     |      no|",    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(false);
  mock().expectOneCall("memfault_log_booted").andReturnValue(false);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(true);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(false);
  memfault_self_test_component_boot_test();
}

TEST(MemfaultSelfTestComponentBooted, Test_ComponentBootAllBooted) {
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component Boot Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Component       | Booted?|",
    "-----------------------------", "Event Storage   |     yes|",
    "Logging         |     yes|",    "Reboot Tracking |     yes|",
    "Trace Event     |     yes|",    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_event_storage_booted").andReturnValue(true);
  mock().expectOneCall("memfault_log_booted").andReturnValue(true);
  mock().expectOneCall("memfault_reboot_tracking_booted").andReturnValue(true);
  mock().expectOneCall("memfault_trace_event_booted").andReturnValue(true);
  memfault_self_test_component_boot_test();
}
