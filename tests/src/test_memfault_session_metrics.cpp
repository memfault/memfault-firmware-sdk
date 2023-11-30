//! @file
//!
//! @brief

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/metrics/reliability.h"
#include "memfault/metrics/serializer.h"

#define FAKE_STORAGE_SIZE 1024

extern "C" {
static uint64_t s_fake_time_ms = 0;
static void prv_fake_time_set(uint64_t new_fake_time_ms) {
  s_fake_time_ms = new_fake_time_ms;
}

static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return s_fake_time_ms;
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

// fake
void memfault_metrics_reliability_collect(void) {}
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MEMFAULT_UNUSED MemfaultPlatformTimerCallback callback) {
  return mock()
    .actualCall(__func__)
    .withParameter("period_sec", period_sec)
    .returnBoolValueOrDefault(true);
}

static void session_end_cb(void) {
  mock().actualCall(__func__);
}

// clang-format off
TEST_GROUP(MemfaultSessionMetrics){
    void setup() {
        static uint8_t s_storage[FAKE_STORAGE_SIZE];

        s_fake_event_storage_impl = memfault_events_storage_boot(&s_storage, sizeof(s_storage));
    }
    void teardown() {
        // Clear session end CBs
        memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                                (MemfaultMetricsSessionEndCb)NULL);
        mock().checkExpectations();
        mock().clear();
    }
};
// clang-format on

TEST(MemfaultSessionMetrics, Test_SessionTimer) {
        MemfaultMetricId key = MEMFAULT_METRICS_KEY(mflt_session_timer_test_key_session);

        uint64_t expected = 100;
        int rv = 0;

        mock().expectOneCall("memfault_metrics_session_serialize");

        rv = MEMFAULT_METRICS_SESSION_START(test_key_session);
        prv_fake_time_set(expected);
        rv = MEMFAULT_METRICS_SESSION_END(test_key_session);
        LONGS_EQUAL(0, rv);

        uint32_t val = 0;
        rv = 0;
        rv = memfault_metrics_heartbeat_timer_read(key, &val);
        LONGS_EQUAL(0, rv);
        LONGS_EQUAL(expected, val);
}

TEST(MemfaultSessionMetrics, Test_StringKey) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_string);
#define TEST_STRING "test_string"

  int rv = memfault_metrics_heartbeat_set_string(key, TEST_STRING);
  LONGS_EQUAL(0, rv);

  char actual[32] = {0};
  rv = memfault_metrics_heartbeat_read_string(key, (char *)actual, sizeof(actual));
  LONGS_EQUAL(0, rv);

  STRCMP_EQUAL(TEST_STRING, (const char *)actual);
}

TEST(MemfaultSessionMetrics, Test_SessionTimerStopBeforeStart) {
  int rv = MEMFAULT_METRICS_SESSION_END(test_key_session);

  LONGS_EQUAL(-4, rv);
}

TEST(MemfaultSessionMetrics, Test_UnexpectedRebootResetOnlyOnHeartbeatStart) {
  MemfaultMetricId reboot_key = MEMFAULT_METRICS_KEY(MemfaultSdkMetric_UnexpectedRebootCount);

  mock().expectOneCall("memfault_metrics_session_serialize");

  memfault_metrics_heartbeat_set_unsigned(reboot_key, 1);
  uint32_t reboot_val = 0;
  int rv = memfault_metrics_heartbeat_read_unsigned(reboot_key, &reboot_val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(1, reboot_val);

  rv = MEMFAULT_METRICS_SESSION_START(test_key_session);

  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(1, reboot_val);

  MEMFAULT_METRICS_SESSION_END(test_key_session);
}

TEST(MemfaultSessionMetrics, Test_RegisterSessionEndCb) {
  mock().expectOneCall("memfault_metrics_session_serialize");
  mock().expectOneCall("session_end_cb");

  memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                           session_end_cb);
  MEMFAULT_METRICS_SESSION_START(test_key_session);
  MEMFAULT_METRICS_SESSION_END(test_key_session);
}

TEST(MemfaultSessionMetrics, Test_UnRegisterSessionEndCb) {
  mock().expectOneCall("memfault_metrics_session_serialize");

  memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                           session_end_cb);
  memfault_metrics_session_register_end_cb(MEMFAULT_METRICS_SESSION_KEY(test_key_session),
                                           (MemfaultMetricsSessionEndCb)NULL);

  MEMFAULT_METRICS_SESSION_START(test_key_session);
  MEMFAULT_METRICS_SESSION_END(test_key_session);
}
