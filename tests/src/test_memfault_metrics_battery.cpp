#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "memfault/core/platform/battery.h"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/battery.h"
#include "memfault/metrics/metrics.h"

static const MemfaultMetricId s_metricskey_battery_discharge_duration_ms =
  MEMFAULT_METRICS_KEY(battery_discharge_duration_ms);
static const MemfaultMetricId s_metricskey_battery_soc_pct_drop =
  MEMFAULT_METRICS_KEY(battery_soc_pct_drop);
static const MemfaultMetricId s_metricskey_battery_soc_pct = MEMFAULT_METRICS_KEY(battery_soc_pct);

extern "C" {
uint32_t memfault_platform_get_stateofcharge(void) {
  return mock().actualCall(__func__).returnUnsignedIntValue();
}

bool memfault_platform_is_discharging(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

// fake instead of mock for simplicity sake
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
}

// clang-format off
TEST_GROUP(BatteryMetrics) {
  void setup(){
    s_fake_time_ms = 0;
    mock().strictOrder();
    static MemfaultMetricIdsComparator s_metric_id_comparator;
    mock().installComparator("MemfaultMetricId", s_metric_id_comparator);
  }
  void teardown(){
    mock().checkExpectations();
    mock().removeAllComparatorsAndCopiers();
    mock().clear();
  }
};
// clang-format on

TEST(BatteryMetrics, Basic) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  prv_fake_time_set(UINT64_MAX - 99);
  memfault_metrics_battery_boot();

  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  // 100ms elapsed (wraps)
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct_drop)
    .withParameter("unsigned_value", 1);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_discharge_duration_ms)
    .withParameter("unsigned_value", 100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
};

TEST(BatteryMetrics, ChargeConnectEvent) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  memfault_metrics_battery_boot();

  // stopped-discharging event
  memfault_metrics_battery_stopped_discharging();

  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(false);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, NegativeDrop) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  memfault_metrics_battery_boot();

  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // -1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 100);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, StateMismatchAtSessionEnd) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  memfault_metrics_battery_boot();

  // return not discharging, without first calling memfault_metrics_battery_stopped_discharging()
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(false);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, StateMismatchAtSessionEnd2) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  memfault_metrics_battery_boot();

  // stopped-discharging event (charger connected)
  memfault_metrics_battery_stopped_discharging();

  // return discharging, so charger was disconnected
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // advance another interval, but charger is still disconnected. the second
  // interval should now be valid.
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // 0% drop!
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct_drop)
    .withParameter("unsigned_value", 0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_discharge_duration_ms)
    .withParameter("unsigned_value", 100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
}

// invalidate first and second interval when charger connects in first interval
// and disconnects in the second interval
TEST(BatteryMetrics, TwoIntervals2) {
  // start with battery discharging
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  memfault_metrics_battery_boot();

  // stopped-discharging event (charger connected)
  memfault_metrics_battery_stopped_discharging();

  // return not-discharging (charger connected)
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(false);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // advance another hour, but charger is still connected. the second hour
  // should also be invalid
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(false);
  // -1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(100);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 100);
  memfault_metrics_battery_collect_data();

  // and advance one more hour, this time with the charger disconnected at the
  // end of the hour
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(99);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // and FINALLY, the 4th hour will be a valid discharging session
  mock().expectOneCall("memfault_platform_is_discharging").andReturnValue(true);
  // 1% drop
  mock().expectOneCall("memfault_platform_get_stateofcharge").andReturnValue(98);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct_drop)
    .withParameter("unsigned_value", 1);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_discharge_duration_ms)
    .withParameter("unsigned_value", 100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 98);
  memfault_metrics_battery_collect_data();
}
