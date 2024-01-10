//! @file
//!
//! @brief

#include <stddef.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_platform_metrics_locking.h"
#include "memfault/components.h"
#include "memfault/metrics/serializer.h"
#include "mocks/mock_memfault_platform_debug_log.h"

extern "C" {
static uint64_t s_boot_time_ms = 0;

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return s_boot_time_ms;
}
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MEMFAULT_UNUSED MemfaultPlatformTimerCallback callback) {
  return mock()
    .actualCall(__func__)
    .withParameter("period_sec", period_sec)
    .returnBoolValueOrDefault(true);
}

bool memfault_metrics_heartbeat_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

bool memfault_metrics_session_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl,
  MEMFAULT_UNUSED eMfltMetricsSessionIndex session_index) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(0);
}

// clang-format off
TEST_GROUP(MemfaultHeartbeatMetricsDebug){
  void setup(){
    s_boot_time_ms = 0;
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};
// clang-format on

void memfault_metrics_heartbeat_collect_data(void) {
  MEMFAULT_METRIC_SET_UNSIGNED(test_key_unsigned, 1234);
  MEMFAULT_METRIC_SET_SIGNED(test_key_signed, -100);
  MEMFAULT_METRIC_SET_STRING(test_key_string, "heyo!");

  // add a call here to ensure it doesn't recurse endlessly. customers may be
  // using this pattern
  memfault_metrics_heartbeat_debug_print();
}

//! check the debug print outputs the correct values depending on metrics state
TEST(MemfaultHeartbeatMetricsDebug, Test_DebugPrints) {
  static uint8_t s_storage[1000];

  mock().expectOneCall("memfault_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  const sMemfaultEventStorageImpl *s_fake_event_storage_impl =
    memfault_events_storage_boot(&s_storage, sizeof(s_storage));
  mock().expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size");

  sMemfaultMetricBootInfo boot_info = {.unexpected_reboot_count = 1};
  int rv = memfault_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(0, rv);
  mock().checkExpectations();

  // this should output the system reset values
  const char *heartbeat_debug_print_on_boot[] = {
    "Heartbeat keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: 1",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  test_key_unsigned: null",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, heartbeat_debug_print_on_boot,
                                 MEMFAULT_ARRAY_SIZE(heartbeat_debug_print_on_boot));
  memfault_metrics_heartbeat_debug_print();
  mock().checkExpectations();

  s_boot_time_ms = 678;
  MEMFAULT_METRIC_TIMER_START(test_key_timer);
  s_boot_time_ms = 5678;

  // debug trigger will update, save, and zero the values
  const char *heartbeat_debug_print_after_collected[] = {
    "Heartbeat keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 5678",
    "  MemfaultSdkMetric_UnexpectedRebootCount: 1",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  test_key_unsigned: 1234",
    "  test_key_signed: -100",
    "  test_key_timer: 5000",
    "  test_key_string: \"heyo!\"",
  };
  mock().expectOneCall("memfault_metrics_reliability_collect");
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info,
                                 heartbeat_debug_print_after_collected,
                                 MEMFAULT_ARRAY_SIZE(heartbeat_debug_print_after_collected));
  mock().expectOneCall("memfault_metrics_heartbeat_serialize");
  memfault_metrics_heartbeat_debug_trigger();
  mock().checkExpectations();

  // after trigger, metrics should be zeroed now
  const char *heartbeat_debug_print_reset[] = {
    "Heartbeat keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  test_key_unsigned: null",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, heartbeat_debug_print_reset,
                                 MEMFAULT_ARRAY_SIZE(heartbeat_debug_print_reset));
  memfault_metrics_heartbeat_debug_print();
  mock().checkExpectations();

  // Finally test that memfault_metrics_heartbeat_add by itself sets the metric
  // to is_set
  const char *heartbeat_add_non_null[] = {
    "Heartbeat keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  test_key_unsigned: 123",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, heartbeat_add_non_null,
                                 MEMFAULT_ARRAY_SIZE(heartbeat_add_non_null));
  // call add on this metric alone and confirm it is set
  MEMFAULT_METRIC_ADD(test_key_unsigned, 123);
  memfault_metrics_heartbeat_debug_print();
  mock().checkExpectations();
}
