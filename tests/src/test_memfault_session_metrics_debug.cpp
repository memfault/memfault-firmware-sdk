//! @file test_memfault_session_metrics_debug.cpp
//!
//! @brief Unit tests for session metrics debug print functions

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

static uint64_t s_boot_time_ms = 0;
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return s_boot_time_ms;
}

#define FAKE_STORAGE_SIZE 1000
static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;
static eMfltMetricsSessionIndex s_current_session_key;

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

static void test_key_session_end_cb(void) {
  // This might be called in the callback, so this use case
  memfault_metrics_session_debug_print(s_current_session_key);
}

static void test_key_session_dummy_end_cb(void) {
  // do nothing
}

TEST_GROUP(MemfaultSessionMetricsDebug) {
  void setup() {
    s_boot_time_ms = 0;
    mock().strictOrder();

    static uint8_t s_storage[FAKE_STORAGE_SIZE];
    s_fake_event_storage_impl = memfault_events_storage_boot(&s_storage, sizeof(s_storage));

    // memfault_metrics_boot() typically starts the heartbeat timer, but call it manually since we
    // can't call memfault_metrics_boot() multiple times
    MEMFAULT_METRIC_TIMER_START(MemfaultSdkMetric_IntervalMs);
  }
  void teardown() {
    // Reset the heartbeat metrics
    MEMFAULT_METRIC_TIMER_STOP(MemfaultSdkMetric_IntervalMs);
    mock().expectOneCall("memfault_metrics_reliability_collect");
    mock().expectOneCall("memfault_metrics_heartbeat_serialize");
    memfault_metrics_heartbeat_debug_trigger();

    // Reset the session metrics and the callback for first session
    memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                             test_key_session_dummy_end_cb);
    MEMFAULT_METRICS_SESSION_START(test_key_session);
    mock().expectOneCall("memfault_metrics_session_serialize");
    MEMFAULT_METRICS_SESSION_END(test_key_session);
    MEMFAULT_METRIC_SESSION_TIMER_STOP(test_timer, test_key_session);

    // Reset session metrics for second session
    MEMFAULT_METRICS_SESSION_START(test_key_session_two);
    mock().expectOneCall("memfault_metrics_session_serialize");
    MEMFAULT_METRICS_SESSION_END(test_key_session_two);
    MEMFAULT_METRIC_SESSION_TIMER_STOP(test_timer, test_key_session_two);

    mock().checkExpectations();
    mock().clear();
  }
};

//! check the debug print outputs the correct values depending on metrics state
TEST(MemfaultSessionMetricsDebug, Test_HeartbeatResetState) {
  // this should output the system reset values
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  MemfaultSDKMetric_log_dropped_lines: null",
    "  MemfaultSDKMetric_log_recorded_lines: null",
    "  test_key_unsigned: null",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
    "  test_unsigned_scale_value: null",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(heartbeat);
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_session_debug_print(s_current_session_key);
}

//! check the debug print outputs the correct values depending on metrics state
TEST(MemfaultSessionMetricsDebug, Test_HeartbeatCollectionUpdateAndReset) {
  s_boot_time_ms = 678;
  MEMFAULT_METRIC_TIMER_START(test_key_timer);
  s_boot_time_ms = 5678;

  MEMFAULT_METRIC_SET_UNSIGNED(test_key_unsigned, 1234);
  MEMFAULT_METRIC_SET_SIGNED(test_key_signed, -100);
  MEMFAULT_METRIC_SET_STRING(test_key_string, "heyo!");

  // debug trigger will update, save, and zero the values
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 5678",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  MemfaultSDKMetric_log_dropped_lines: null",
    "  MemfaultSDKMetric_log_recorded_lines: null",
    "  test_key_unsigned: 1234",
    "  test_key_signed: -100",
    "  test_key_timer: 5000",
    "  test_key_string: \"heyo!\"",
    "  test_unsigned_scale_value: null",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(heartbeat);
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_session_debug_print(s_current_session_key);

  // Clean up. Intentionally do not call timer stop before printing the heartbeat to make sure
  // we check that we have forced timer updates in our debug print function
  MEMFAULT_METRIC_TIMER_STOP(test_key_timer);

  // Force a heartbeat metrics reset
  mock().expectOneCall("memfault_metrics_reliability_collect");
  mock().expectOneCall("memfault_metrics_heartbeat_serialize");
  memfault_metrics_heartbeat_debug_trigger();

  // after trigger, metrics should be zeroed now
  const char *expected_debug_print_reset[] = {
    "Metrics keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  MemfaultSDKMetric_log_dropped_lines: null",
    "  MemfaultSDKMetric_log_recorded_lines: null",
    "  test_key_unsigned: null",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
    "  test_unsigned_scale_value: null",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print_reset,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print_reset));
  memfault_metrics_session_debug_print(s_current_session_key);
}

//! check for correct result when printing heartbeat metrics for a simple add
TEST(MemfaultSessionMetricsDebug, Test_HeartbeatUpdateAdd) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  MemfaultSdkMetric_IntervalMs: 0",
    "  MemfaultSdkMetric_UnexpectedRebootCount: null",
    "  operational_hours: null",
    "  operational_crashfree_hours: null",
    "  MemfaultSDKMetric_log_dropped_lines: null",
    "  MemfaultSDKMetric_log_recorded_lines: null",
    "  test_key_unsigned: 123",
    "  test_key_signed: null",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
    "  test_unsigned_scale_value: null",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(heartbeat);
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  // call add on this metric alone and confirm it is set
  MEMFAULT_METRIC_ADD(test_key_unsigned, 123);
  memfault_metrics_session_debug_print(s_current_session_key);
}

//! check for correct result when printing session metrics that have been reset
TEST(MemfaultSessionMetricsDebug, Test_SessionResetState) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  test_key_session__MemfaultSdkMetric_IntervalMs: 0",
    "  test_key_session__operational_crashes: null",
    "  test_key_session__test_unsigned: null",
    "  test_key_session__test_string: \"\"",
    "  test_key_session__test_timer: 0",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(test_key_session);
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_session_debug_print(s_current_session_key);
}

//! check for correct result when printing session metrics after session has started, but only
//! the session timer is updated
TEST(MemfaultSessionMetricsDebug, Test_SessionTimerUpdateState) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  test_key_session__MemfaultSdkMetric_IntervalMs: 5000",
    "  test_key_session__operational_crashes: null",
    "  test_key_session__test_unsigned: null",
    "  test_key_session__test_string: \"\"",
    "  test_key_session__test_timer: 5000",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(test_key_session);
  memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                           test_key_session_end_cb);

  s_boot_time_ms = 678;
  MEMFAULT_METRICS_SESSION_START(test_key_session);
  MEMFAULT_METRIC_SESSION_TIMER_START(test_timer, test_key_session);
  s_boot_time_ms = 5678;

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_session_debug_print(MEMFAULT_METRICS_SESSION_KEY(test_key_session));
}

//! check for correct result when printing session metrics before and after a session ends
TEST(MemfaultSessionMetricsDebug, Test_SessionUpdateState) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  test_key_session__MemfaultSdkMetric_IntervalMs: 5000",
    "  test_key_session__operational_crashes: null",
    "  test_key_session__test_unsigned: 35",
    "  test_key_session__test_string: \"sessions!\"",
    "  test_key_session__test_timer: 0",
  };
  s_current_session_key = MEMFAULT_METRICS_SESSION_KEY(test_key_session);
  memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                           test_key_session_end_cb);

  s_boot_time_ms = 678;
  MEMFAULT_METRICS_SESSION_START(test_key_session);
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(test_unsigned, test_key_session, 35);
  MEMFAULT_METRIC_SESSION_SET_STRING(test_string, test_key_session, "sessions!");
  s_boot_time_ms = 5678;

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_session_debug_print(s_current_session_key);

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  mock().expectOneCall("memfault_metrics_session_serialize");
  MEMFAULT_METRICS_SESSION_END(test_key_session);
}

//! check for correct result when printing all session metrics that have been reset
TEST(MemfaultSessionMetricsDebug, Test_AllSessionsResetState) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  test_key_session__MemfaultSdkMetric_IntervalMs: 0",
    "  test_key_session__operational_crashes: null",
    "  test_key_session__test_unsigned: null",
    "  test_key_session__test_string: \"\"",
    "  test_key_session__test_timer: 0",
    "  test_key_session_two__MemfaultSdkMetric_IntervalMs: 0",
    "  test_key_session_two__operational_crashes: null",
    "  test_key_session_two__test_unsigned: null",
    "  test_key_session_two__test_string: \"\"",
    "  test_key_session_two__test_timer: 0",
    "  test_key_session_two__test_signed_scale_value: null",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_all_sessions_debug_print();
}

//! check for correct result when printing session metrics after session has started, but only
//! the session timer is updated
TEST(MemfaultSessionMetricsDebug, Test_AllSessionsTimerUpdateState) {
  const char *expected_debug_print[] = {
    "Metrics keys/values:",
    "  test_key_session__MemfaultSdkMetric_IntervalMs: 5000",
    "  test_key_session__operational_crashes: null",
    "  test_key_session__test_unsigned: null",
    "  test_key_session__test_string: \"\"",
    "  test_key_session__test_timer: 5000",
    "  test_key_session_two__MemfaultSdkMetric_IntervalMs: 5000",
    "  test_key_session_two__operational_crashes: null",
    "  test_key_session_two__test_unsigned: null",
    "  test_key_session_two__test_string: \"\"",
    "  test_key_session_two__test_timer: 5000",
    "  test_key_session_two__test_signed_scale_value: null",
  };

  s_boot_time_ms = 678;
  MEMFAULT_METRICS_SESSION_START(test_key_session);
  MEMFAULT_METRIC_SESSION_TIMER_START(test_timer, test_key_session);
  MEMFAULT_METRICS_SESSION_START(test_key_session_two);
  MEMFAULT_METRIC_SESSION_TIMER_START(test_timer, test_key_session_two);
  s_boot_time_ms = 5678;

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, expected_debug_print,
                                 MEMFAULT_ARRAY_SIZE(expected_debug_print));
  memfault_metrics_all_sessions_debug_print();
}
