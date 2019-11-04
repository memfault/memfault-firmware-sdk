//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>

  #include "fakes/fake_memfault_platform_metrics_locking.h"
  #include "memfault/metrics/metrics.h"
  #include "memfault/metrics/platform/memfault_platform_timer.h"
  #include "memfault/metrics/platform/overrides.h"
  #include "memfault/metrics/serializer.h"

  int memfault_platform_timer_create(void **timer_data) {
    return 0;
  }

  int memfault_platform_timer_start(void *timer_data, uint32_t period_s,
                                    MemfaultPlatformTimerCb cb) {
    return 0;
  }

  bool memfault_metrics_heartbeat_serialize(void) {
    return true;
  }
}

TEST_GROUP(MemfaultHeartbeatMetrics){
  void setup() {
    fake_memfault_metrics_platorm_locking_reboot();
    memfault_metrics_boot();
  }
  void teardown() {
    // dump the final result & also sanity test that this routine works
    memfault_metrics_heartbeat_debug_print();
    CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
    mock().checkExpectations();
    mock().clear();
  }
};

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

  rv = memfault_metrics_heartbeat_set_unsigned(key, 100);
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


TEST(MemfaultHeartbeatMetrics, Test_KeyDNE) {
  // NOTE: Using the macro MEMFAULT_METRICS_KEY, it's impossible for a non-existent key to trigger a
  // compilation error
  MemfaultMetricId key = (MemfaultMetricId){ "non_existent_key" };

  int rv = memfault_metrics_heartbeat_set_signed(key, 0);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_set_unsigned(key, 0);
  CHECK(rv != 0);
  rv = memfault_metrics_heartbeat_add(key, INT32_MIN);
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

  memfault_metrics_heartbeat_read_signed(keyi32, &vali32);
  memfault_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(vali32, 200);
  LONGS_EQUAL(valu32, 199);

  mock().expectOneCall("memfault_metrics_heartbeat_collect_data");
  memfault_metrics_heartbeat_debug_trigger();
  mock().checkExpectations();

  // values should all be reset
  memfault_metrics_heartbeat_read_signed(keyi32, &vali32);
  memfault_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(vali32, 0);
  LONGS_EQUAL(valu32, 0);
}
