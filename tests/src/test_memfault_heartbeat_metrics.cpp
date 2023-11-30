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
#include "memfault/core/compiler.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/overrides.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/metrics/serializer.h"
#include "memfault/metrics/utils.h"

extern "C" {
static void (*s_serializer_check_cb)(void) = NULL;

static uint64_t s_fake_time_ms = 0;
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return s_fake_time_ms;
}

static void prv_fake_time_set(uint64_t new_fake_time_ms) {
  s_fake_time_ms = new_fake_time_ms;
}

static void prv_fake_time_incr(uint64_t fake_time_delta_ms) {
  s_fake_time_ms += fake_time_delta_ms;
}

static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MEMFAULT_UNUSED MemfaultPlatformTimerCallback callback) {
  return mock()
    .actualCall(__func__)
    .withParameter("period_sec", period_sec)
    .returnBoolValueOrDefault(true);
}

#define FAKE_STORAGE_SIZE 100

bool memfault_metrics_heartbeat_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl) {
  mock().actualCall(__func__);
  if (s_serializer_check_cb != NULL) {
    s_serializer_check_cb();
  }

  return true;
}

bool memfault_metrics_session_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl,
  MEMFAULT_UNUSED eMfltMetricsSessionIndex session_index) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(FAKE_STORAGE_SIZE);
}

// clang-format off
TEST_GROUP(MemfaultHeartbeatMetrics){
  void setup(){
    s_fake_time_ms = 0;
    s_serializer_check_cb = NULL;
    fake_memfault_metrics_platorm_locking_reboot();
    static uint8_t s_storage[FAKE_STORAGE_SIZE];
    mock().strictOrder();

    // Check that by default the heartbeat interval is once / hour
    mock().expectOneCall("memfault_platform_metrics_timer_boot").withParameter("period_sec", 3600);

    s_fake_event_storage_impl = memfault_events_storage_boot(&s_storage, sizeof(s_storage));
    mock().expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size");

    sMemfaultMetricBootInfo boot_info = { .unexpected_reboot_count = 7 };
    int rv = memfault_metrics_boot(s_fake_event_storage_impl, &boot_info);
    LONGS_EQUAL(0, rv);
    mock().checkExpectations();

    // crash count should have beem copied into heartbeat
    uint32_t logged_crash_count = 0;
    memfault_metrics_heartbeat_read_unsigned(
      MEMFAULT_METRICS_KEY(MemfaultSdkMetric_UnexpectedRebootCount), &logged_crash_count);
    LONGS_EQUAL(boot_info.unexpected_reboot_count, logged_crash_count);

    // IntervalMs & RebootCount & operational_hours * operational_crashfree_hours
    const size_t num_memfault_sdk_metrics = 4;

    // We should test all the types of available metrics so if this
    // fails it means there's a new type we aren't yet covering
    LONGS_EQUAL(kMemfaultMetricType_NumTypes + num_memfault_sdk_metrics,
                memfault_metrics_heartbeat_get_num_metrics());
    }
    void teardown() {
      // dump the final result & also sanity test that this routine works
      memfault_metrics_heartbeat_debug_print();
      CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
      // we are simulating a reboot, so the timer should be running.
      // Let's stop it and confirm that works and reset our state so the next
      // boot call will succeed.
      MemfaultMetricId id = MEMFAULT_METRICS_KEY(MemfaultSdkMetric_IntervalMs);
      int rv = memfault_metrics_heartbeat_timer_stop(id);
      LONGS_EQUAL(0, rv);
      mock().checkExpectations();
      mock().clear();
    }
};
// clang-format on

TEST(MemfaultHeartbeatMetrics, Test_BootStorageTooSmall) {
  // Check that by default the heartbeat interval is once / hour
  mock().expectOneCall("memfault_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  // reboot metrics with storage that is too small to actually hold an event
  // this should result in a warning being emitted
  mock()
    .expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size")
    .andReturnValue(FAKE_STORAGE_SIZE + 1);

  sMemfaultMetricBootInfo boot_info = {.unexpected_reboot_count = 1};
  int rv = memfault_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(-5, rv);
  mock().checkExpectations();
}

TEST(MemfaultHeartbeatMetrics, Test_TimerInitFailed) {
  // Nothing else should happen if timer initialization failed for some reason
  mock()
    .expectOneCall("memfault_platform_metrics_timer_boot")
    .withParameter("period_sec", 3600)
    .andReturnValue(false);

  sMemfaultMetricBootInfo boot_info = {.unexpected_reboot_count = 1};
  int rv = memfault_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(-6, rv);
  mock().checkExpectations();
}

TEST(MemfaultHeartbeatMetrics, Test_UnsignedHeartbeatValue) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_unsigned);
  int rv = memfault_metrics_heartbeat_set_unsigned(key, 100);
  LONGS_EQUAL(0, rv);

  rv = memfault_metrics_heartbeat_set_signed(key, 100);
  CHECK(rv != 0);

  rv = memfault_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_add(key, 2);
  LONGS_EQUAL(0, rv);
  uint32_t val = 0;
  rv = memfault_metrics_heartbeat_read_unsigned(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(104, val);

  // test clipping
  memfault_metrics_heartbeat_add(key, INT32_MAX);
  memfault_metrics_heartbeat_add(key, INT32_MAX);
  rv = memfault_metrics_heartbeat_read_unsigned(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(UINT32_MAX, val);
}

TEST(MemfaultHeartbeatMetrics, Test_SignedHeartbeatValue) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_signed);
  int rv = memfault_metrics_heartbeat_set_signed(key, -100);
  LONGS_EQUAL(0, rv);

  // try wrong types
  rv = memfault_metrics_heartbeat_set_unsigned(key, 100);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  rv = memfault_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_add(key, 2);
  LONGS_EQUAL(0, rv);
  int32_t val = 0;
  rv = memfault_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(-96, val);

  memfault_metrics_heartbeat_add(key, INT32_MAX);
  memfault_metrics_heartbeat_add(key, INT32_MAX);
  rv = memfault_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(INT32_MAX, val);

  memfault_metrics_heartbeat_set_signed(key, -100);
  memfault_metrics_heartbeat_add(key, INT32_MIN);
  rv = memfault_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(INT32_MIN, val);
}

TEST(MemfaultHeartbeatMetrics, Test_TimerHeartBeatValueSimple) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);
  // no-op
  int rv = memfault_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  // start the timer
  rv = memfault_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_incr(10);
  // no-op
  rv = memfault_metrics_heartbeat_timer_start(key);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_add(key, 20);
  CHECK(rv != 0);

  rv = memfault_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(10, val);
}

TEST(MemfaultHeartbeatMetrics, Test_TimerHeartBeatReadWhileRunning)
{
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);

  // start the timer
  int rv = memfault_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);

  // read while running
  uint32_t val;
  prv_fake_time_incr(10);
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(10, val);

  // stop the timer
  prv_fake_time_incr(9);
  rv = memfault_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(19, val);
}

TEST(MemfaultHeartbeatMetrics, Test_TimerHeartBeatValueRollover) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);

  prv_fake_time_set(0x80000000 - 9);

  int rv = memfault_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_set(0x80000008);

  rv = memfault_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(17, val);
}

TEST(MemfaultHeartbeatMetrics, Test_TimerHeartBeatNoChange) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);

  int rv = memfault_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);

  rv = memfault_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);
}

#define EXPECTED_HEARTBEAT_TIMER_VAL_MS 13

static void prv_serialize_check_cb(void) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);
  uint32_t val;
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(EXPECTED_HEARTBEAT_TIMER_VAL_MS, val);
}

TEST(MemfaultHeartbeatMetrics, Test_TimerActiveWhenHeartbeatCollected) {
  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_timer);

  int rv = memfault_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);

  s_serializer_check_cb = &prv_serialize_check_cb;
  mock().expectOneCall("memfault_metrics_reliability_collect");
  mock().expectOneCall("memfault_metrics_heartbeat_collect_data");
  mock().expectOneCall("memfault_metrics_heartbeat_serialize");
  memfault_metrics_heartbeat_debug_trigger();

  uint32_t val;
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);

  // timer should still be running
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);
  rv = memfault_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);
  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(EXPECTED_HEARTBEAT_TIMER_VAL_MS, val);

  // the time is no longer running so an increment shouldn't be counted
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);
  s_serializer_check_cb = &prv_serialize_check_cb;
  mock().expectOneCall("memfault_metrics_reliability_collect");
  mock().expectOneCall("memfault_metrics_heartbeat_collect_data");
  mock().expectOneCall("memfault_metrics_heartbeat_serialize");
  memfault_metrics_heartbeat_debug_trigger();

  memfault_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);
}

TEST(MemfaultHeartbeatMetrics, Test_String) {
#define SAMPLE_STRING "0123456789abcdef"
  static_assert(__builtin_strlen(SAMPLE_STRING) == 16,
                "be sure to modify tests/stub_includes/memfault_metrics_heartbeat_config.def to "
                "match exactly, so we can check for buffer overflows!");

  MemfaultMetricId key = MEMFAULT_METRICS_KEY(test_key_string);

  // just the correct size
  {
    int rv = memfault_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) + 1];
    memset(sample_string, 0, sizeof(sample_string));

    rv = memfault_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SAMPLE_STRING, (const char *)sample_string);
  }

  // set too long a string
  {
    int rv = memfault_metrics_heartbeat_set_string(key, SAMPLE_STRING "1");
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) + 1];
    memset(sample_string, 0, sizeof(sample_string));

    rv = memfault_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SAMPLE_STRING, (const char *)sample_string);
  }

  // read with bad destination buffer
  {
    int rv = memfault_metrics_heartbeat_read_string(key, NULL, 0);
    CHECK(rv != 0);
  }

  // write with longer then shorter string and confirm readback is ok
  {
    int rv = memfault_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);
#define SHORT_TEST_STRING "12"
    rv = memfault_metrics_heartbeat_set_string(key, SHORT_TEST_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SHORT_TEST_STRING)];
    memset(sample_string, 0, sizeof(sample_string));

    rv = memfault_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SHORT_TEST_STRING, (const char *)sample_string);
  }

  // read with a buffer that's too small, and confirm it's a valid string
  {
    int rv = memfault_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) - 1];
    memset(sample_string, 'a', sizeof(sample_string));

    rv = memfault_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL("0123456789abcde", (const char *)sample_string);
  }
}

TEST(MemfaultHeartbeatMetrics, Test_BadBoot) {
  sMemfaultMetricBootInfo info = {.unexpected_reboot_count = 1};

  int rv = memfault_metrics_boot(NULL, &info);
  LONGS_EQUAL(-3, rv);

  rv = memfault_metrics_boot(s_fake_event_storage_impl, NULL);
  LONGS_EQUAL(-3, rv);

  rv = memfault_metrics_boot(NULL, NULL);
  LONGS_EQUAL(-3, rv);

  // calling boot with valid args twice in a row should fail (interval timer already running)
  mock().expectOneCall("memfault_platform_metrics_timer_boot").withParameter("period_sec", 3600);
  mock().expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size");
  rv = memfault_metrics_boot(s_fake_event_storage_impl, &info);
  LONGS_EQUAL(-4, rv);
}

TEST(MemfaultHeartbeatMetrics, Test_KeyDNE) {
  // NOTE: Using the macro MEMFAULT_METRICS_KEY, it's impossible for a non-existent key to trigger a
  // compilation error so we just create an invalid key.
  MemfaultMetricId key = (MemfaultMetricId){INT32_MAX};

  int rv = memfault_metrics_heartbeat_set_signed(key, 0);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_set_unsigned(key, 0);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_add(key, INT32_MIN);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_set_string(key, NULL);
  CHECK(rv != 0);

  rv = memfault_metrics_heartbeat_timer_start(key);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  int32_t vali32;
  rv = memfault_metrics_heartbeat_read_signed(key, NULL);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_read_signed(key, &vali32);
  CHECK(rv != 0);

  uint32_t valu32;
  rv = memfault_metrics_heartbeat_read_unsigned(key, NULL);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_read_unsigned(key, &valu32);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_timer_read(key, NULL);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_timer_read(key, &valu32);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_read_string(key, NULL, 0);
  CHECK(rv != 0);
}

void memfault_metrics_heartbeat_collect_data(void) {
  mock().actualCall(__func__);
}

TEST(MemfaultHeartbeatMetrics, Test_HeartbeatCollection) {
  MemfaultMetricId keyi32 = MEMFAULT_METRICS_KEY(test_key_signed);
  MemfaultMetricId keyu32 = MEMFAULT_METRICS_KEY(test_key_unsigned);

  memfault_metrics_heartbeat_set_signed(keyi32, 200);
  memfault_metrics_heartbeat_set_unsigned(keyu32, 199);

  int32_t vali32;
  uint32_t valu32;

  // should fail if we read the wrong type
  int rv = memfault_metrics_heartbeat_read_signed(keyu32, &vali32);
  CHECK(rv != 0);

  rv = memfault_metrics_heartbeat_read_signed(keyi32, &vali32);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(0, rv);

  LONGS_EQUAL(vali32, 200);
  LONGS_EQUAL(valu32, 199);

  mock().expectOneCall("memfault_metrics_reliability_collect");
  mock().expectOneCall("memfault_metrics_heartbeat_collect_data");
  mock().expectOneCall("memfault_metrics_heartbeat_serialize");
  memfault_metrics_heartbeat_debug_trigger();
  mock().checkExpectations();

  // values should all be reset
  rv = memfault_metrics_heartbeat_read_signed(keyi32, &vali32);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  CHECK(rv != 0);
}

// Sanity test the alternate setter API
TEST(MemfaultHeartbeatMetrics, Test_HeartbeatCollectionAltSetter) {
  MemfaultMetricId keyi32 = MEMFAULT_METRICS_KEY(test_key_signed);
  MemfaultMetricId keyu32 = MEMFAULT_METRICS_KEY(test_key_unsigned);
  MemfaultMetricId keytimer = MEMFAULT_METRICS_KEY(test_key_timer);
  MemfaultMetricId keystring = MEMFAULT_METRICS_KEY(test_key_string);

  // integers
  MEMFAULT_METRIC_SET_SIGNED(test_key_signed, -200);
  MEMFAULT_METRIC_SET_UNSIGNED(test_key_unsigned, 199);
  MEMFAULT_METRIC_ADD(test_key_signed, 1);
  MEMFAULT_METRIC_ADD(test_key_unsigned, 1);

  // string
  MEMFAULT_METRIC_SET_STRING(test_key_string, "test");

  // timer
  MEMFAULT_METRIC_TIMER_START(test_key_timer);
  prv_fake_time_incr(10);
  MEMFAULT_METRIC_TIMER_STOP(test_key_timer);

  int32_t vali32;
  uint32_t valu32;
  uint32_t timer_valu32;

  int rv;
  rv = memfault_metrics_heartbeat_read_signed(keyi32, &vali32);
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(0, rv);
  char buf[512];
  rv = memfault_metrics_heartbeat_read_string(keystring, buf, sizeof(buf));
  LONGS_EQUAL(0, rv);
  rv = memfault_metrics_heartbeat_timer_read(keytimer, &timer_valu32);
  LONGS_EQUAL(0, rv);

  LONGS_EQUAL(vali32, -199);
  LONGS_EQUAL(valu32, 200);
  STRCMP_EQUAL(buf, "test");
  LONGS_EQUAL(timer_valu32, 10);
}
