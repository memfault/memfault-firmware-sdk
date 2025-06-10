//! @file
//!
//! @brief

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/metrics/reliability.h"
#include "memfault/metrics/serializer.h"

#define FAKE_STORAGE_SIZE 1024

extern "C" {
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return 0;
}

bool memfault_metrics_heartbeat_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl) {
  return (size_t)mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

bool memfault_metrics_session_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl,
  MEMFAULT_UNUSED eMfltMetricsSessionIndex session_index) {
  return (size_t)mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(FAKE_STORAGE_SIZE);
}
void memfault_reboot_tracking_metrics_session(bool active, uint32_t index) {
  mock().actualCall(__func__).withParameter("active", active).withParameter("index", index);
}

void memfault_reboot_tracking_clear_metrics_sessions(void) {
  mock().actualCall(__func__);
}

bool memfault_reboot_tracking_metrics_session_was_active(uint32_t index) {
  return mock().actualCall(__func__).withParameter("index", index).returnBoolValueOrDefault(true);
}

// fakes
void memfault_metrics_reliability_boot(sMemfaultMetricsReliabilityCtx *ctx) {
  (void)ctx;
}
void memfault_metrics_reliability_collect(void) { }
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MEMFAULT_UNUSED MemfaultPlatformTimerCallback callback) {
  return mock()
    .actualCall(__func__)
    .withParameter("period_sec", period_sec)
    .returnBoolValueOrDefault(true);
}

TEST_GROUP(SessionVitals) {
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    MemfaultMetricId id = MEMFAULT_METRICS_KEY(MemfaultSdkMetric_IntervalMs);
    int rv = memfault_metrics_heartbeat_timer_stop(id);
    LONGS_EQUAL(0, rv);

    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_call_metrics_boot(uint32_t unexpected_reboot_count) {
  static uint8_t s_storage[2048];

  static const sMemfaultEventStorageImpl *fake_event_storage_impl =
    memfault_events_storage_boot(&s_storage, sizeof(s_storage));

  sMemfaultMetricBootInfo boot_info = { .unexpected_reboot_count = unexpected_reboot_count };
  int rv = memfault_metrics_boot(fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(0, rv);
}

// 1. "Happy" path test sequence is:
//    1. start a session to mark it as active in reboot tracking
//    2. boot metrics with unexpected_reboot=true
//    3. confirm the session is serialized out with operation_crashes = 1
TEST(SessionVitals, Test_SessionOperationalCrashes) {
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session")
    .withParameter("active", true)
    .withParameter("index", 0);
  MEMFAULT_METRICS_SESSION_START(test_key_session);

  mock()
    .expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size")
    .andReturnValue(FAKE_STORAGE_SIZE);
  bool unexpected_reboot = true;
  mock()
    .expectOneCall("memfault_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);
  // test_key_session bit
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session_was_active")
    .withParameter("index", 0)
    .andReturnValue(true);
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session")
    .withParameter("active", true)
    .withParameter("index", 0);
  mock().expectOneCall("memfault_metrics_session_serialize");
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session")
    .withParameter("active", false)
    .withParameter("index", 0);

  // test_key_session_two bit
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session_was_active")
    .withParameter("index", 1)
    .andReturnValue(false);

  mock().expectOneCall("memfault_reboot_tracking_clear_metrics_sessions");

  mock()
    .expectOneCall("memfault_platform_metrics_timer_boot")
    .withParameter("period_sec", MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS)
    .andReturnValue(true);

  prv_call_metrics_boot(1);
}

// 2. Alternate ending:
//    1. start a session to mark it as active in reboot tracking
//    2. boot metrics with unexpected_reboot=false
//    3. confirm no session is serialized, and session_active bit is cleared
TEST(SessionVitals, Test_SessionOperationalCrashesExpectedReboot) {
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session")
    .withParameter("active", true)
    .withParameter("index", 0);
  MEMFAULT_METRICS_SESSION_START(test_key_session);

  mock()
    .expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size")
    .andReturnValue(FAKE_STORAGE_SIZE);
  bool unexpected_reboot = false;
  mock()
    .expectOneCall("memfault_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);
  mock().expectOneCall("memfault_reboot_tracking_clear_metrics_sessions");
  mock()
    .expectOneCall("memfault_platform_metrics_timer_boot")
    .withParameter("period_sec", MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS)
    .andReturnValue(true);

  prv_call_metrics_boot(0);
}

// 3. And:
//    1. report a session as inactive in reboot tracking
//    2. boot metrics with unexpected_reboot=true
//    3. confirm no session is serialized, and session_active bit is cleared
TEST(SessionVitals, Test_SessionOperationalCrashesInactive) {
  mock()
    .expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size")
    .andReturnValue(FAKE_STORAGE_SIZE);
  bool unexpected_reboot = true;
  mock()
    .expectOneCall("memfault_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);
  // test_key_session bit
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session_was_active")
    .withParameter("index", 0)
    .andReturnValue(false);

  // test_key_session_two bit
  mock()
    .expectOneCall("memfault_reboot_tracking_metrics_session_was_active")
    .withParameter("index", 1)
    .andReturnValue(false);

  mock().expectOneCall("memfault_reboot_tracking_clear_metrics_sessions");

  mock()
    .expectOneCall("memfault_platform_metrics_timer_boot")
    .withParameter("period_sec", MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS)
    .andReturnValue(true);

  prv_call_metrics_boot(1);
}
