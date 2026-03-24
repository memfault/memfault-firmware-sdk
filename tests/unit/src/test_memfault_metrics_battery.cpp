#include <algorithm>
#include <iostream>
#include <vector>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/battery.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/battery.h"
static const MemfaultMetricId s_metricskey_battery_discharge_duration_ms =
  MEMFAULT_METRICS_KEY(battery_discharge_duration_ms);
static const MemfaultMetricId s_metricskey_battery_soc_pct_drop =
  MEMFAULT_METRICS_KEY(battery_soc_pct_drop);
static const MemfaultMetricId s_metricskey_battery_soc_pct = MEMFAULT_METRICS_KEY(battery_soc_pct);

extern "C" {
int memfault_platform_get_stateofcharge(sMfltPlatformBatterySoc *soc) {
  return mock().actualCall(__func__).withOutputParameter("soc", soc).returnIntValueOrDefault(0);
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

// helper for setting up memfault_platform_get_stateofcharge mock invocations.
// allocates a structure with the specified soc and discharge state, and adds it
// to a vector for later cleanup, then sets the mock.

// vector of allocated sMfltPlatformBatterySoc structs, for later cleanup
static std::vector<void *> s_soc_structs;
static void prv_mock_memfault_platform_get_stateofcharge(uint32_t soc, bool discharging,
                                                         int ret = 0) {
  sMfltPlatformBatterySoc *soc_struct =
    (sMfltPlatformBatterySoc *)malloc(sizeof(sMfltPlatformBatterySoc));
  s_soc_structs.push_back(soc_struct);

  soc_struct->soc = soc;
  soc_struct->discharging = discharging;
  mock()
    .expectOneCall("memfault_platform_get_stateofcharge")
    .withOutputParameterReturning("soc", soc_struct, sizeof(sMfltPlatformBatterySoc))
    .andReturnValue(ret);
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

    // clean up s_soc_structs by freeing all elements. print for each one!
    std::for_each(s_soc_structs.begin(), s_soc_structs.end(), [](void* n) { free(n); });
    s_soc_structs.clear();
    s_soc_structs.shrink_to_fit();
  }
};
// clang-format on

TEST(BatteryMetrics, Basic) {
  // start with battery discharging
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  prv_fake_time_set(UINT64_MAX - 99);
  memfault_metrics_battery_boot();

  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, true);
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
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  memfault_metrics_battery_boot();

  // stopped-discharging event
  memfault_metrics_battery_stopped_discharging();

  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, false);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, NegativeDrop) {
  // start with battery discharging
  prv_mock_memfault_platform_get_stateofcharge(99, true);
  memfault_metrics_battery_boot();

  // -1% drop
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 100);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, StateMismatchAtSessionEnd) {
  // start with battery discharging
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  memfault_metrics_battery_boot();

  // return not discharging, without first calling memfault_metrics_battery_stopped_discharging()
  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, false);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, StateMismatchAtSessionEnd2) {
  // start with battery discharging
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  memfault_metrics_battery_boot();

  // stopped-discharging event (charger connected)
  memfault_metrics_battery_stopped_discharging();

  // return discharging, so charger was disconnected
  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, true);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // advance another interval, but charger is still disconnected. the second
  // interval should now be valid.
  // 0% drop!
  prv_mock_memfault_platform_get_stateofcharge(99, true);
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
  prv_mock_memfault_platform_get_stateofcharge(100, true);
  memfault_metrics_battery_boot();

  // stopped-discharging event (charger connected)
  memfault_metrics_battery_stopped_discharging();

  // return not-discharging (charger connected)
  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, false);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // advance another hour, but charger is still connected. the second hour
  // should also be invalid
  // -1% drop
  prv_mock_memfault_platform_get_stateofcharge(100, false);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 100);
  memfault_metrics_battery_collect_data();

  // and advance one more hour, this time with the charger disconnected at the
  // end of the hour
  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(99, true);
  // 100ms elapsed
  prv_fake_time_incr(100);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 99);
  memfault_metrics_battery_collect_data();

  // and FINALLY, the 4th hour will be a valid discharging session
  // 1% drop
  prv_mock_memfault_platform_get_stateofcharge(98, true);
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

TEST(BatteryMetrics, PerMilleSOC) {
  // just for fun, show that per-mille works correctly too
  // start with battery discharging
  prv_mock_memfault_platform_get_stateofcharge(1000, true);
  memfault_metrics_battery_boot();

  // .1% drop
  prv_mock_memfault_platform_get_stateofcharge(999, true);
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
    .withParameter("unsigned_value", 999);
  memfault_metrics_battery_collect_data();
}

TEST(BatteryMetrics, InvalidSOC) {
  // start with battery at invalid soc
  prv_mock_memfault_platform_get_stateofcharge(1234, true, -1);
  memfault_metrics_battery_boot();

  // invalid at collection time too
  prv_mock_memfault_platform_get_stateofcharge(1234, true, -1);
  memfault_metrics_battery_collect_data();

  // now valid; but interval is invalid, so should not be recorded
  prv_mock_memfault_platform_get_stateofcharge(0, true);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 0);
  memfault_metrics_battery_collect_data();

  // Now set current soc to a weird (but permitted) value
  prv_mock_memfault_platform_get_stateofcharge(101, true);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_metricskey_battery_soc_pct)
    .withParameter("unsigned_value", 101);
  memfault_metrics_battery_collect_data();

  // now finally, a valid interval
  prv_mock_memfault_platform_get_stateofcharge(100, true);
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
    .withParameter("unsigned_value", 100);
  memfault_metrics_battery_collect_data();
}
