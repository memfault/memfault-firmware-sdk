

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "memfault/metrics/connectivity.h"
#include "memfault/metrics/metrics.h"

static MemfaultMetricIdsComparator s_metric_id_comparator;

// These have to have global scope, so the test teardown can access them
static MemfaultMetricId sync_successful_key = MEMFAULT_METRICS_KEY(sync_successful);
static MemfaultMetricId sync_failure_key = MEMFAULT_METRICS_KEY(sync_failure);
static MemfaultMetricId memfault_sync_successful_key =
  MEMFAULT_METRICS_KEY(sync_memfault_successful);
static MemfaultMetricId memfault_sync_failure_key = MEMFAULT_METRICS_KEY(sync_memfault_failure);
static MemfaultMetricId connectivity_connected_time_ms_key =
  MEMFAULT_METRICS_KEY(connectivity_connected_time_ms);
static MemfaultMetricId connectivity_expected_time_ms_key =
  MEMFAULT_METRICS_KEY(connectivity_expected_time_ms);

// clang-format off
TEST_GROUP(MemfaultMetricsConnectivity){
  void setup() {
    mock().strictOrder();
    mock().installComparator("MemfaultMetricId", s_metric_id_comparator);
  }
  void teardown() {
    mock().checkExpectations();
    mock().removeAllComparatorsAndCopiers();
    mock().clear();
  }
};
// clang-format on

TEST(MemfaultMetricsConnectivity, Test_SyncMetrics) {
  // call the basic functions
  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &sync_successful_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  memfault_metrics_connectivity_record_sync_success();

  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &sync_failure_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  memfault_metrics_connectivity_record_sync_failure();

  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &memfault_sync_successful_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  memfault_metrics_connectivity_record_memfault_sync_success();

  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &memfault_sync_failure_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  memfault_metrics_connectivity_record_memfault_sync_failure();
}

// Tests for memfault_metrics_connectivity_connected_state_change
TEST(MemfaultMetricsConnectivity, Test_ConnectedStateChange) {
  // walk through the state transitions

  // start
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(kMemfaultMetricsConnectivityState_Started);

  // connect
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_connected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(kMemfaultMetricsConnectivityState_Connected);

  // disconnect
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_stop")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_connected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(
    kMemfaultMetricsConnectivityState_ConnectionLost);

  // stop
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_stop")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_connected_time_ms_key)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_stop")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(kMemfaultMetricsConnectivityState_Stopped);

  // junk value, should be ignored
  memfault_metrics_connectivity_connected_state_change((eMemfaultMetricsConnectivityState)-1);

  // test connection_lost->started->connected too. expected timer should run the whole time
  // connection_lost
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_stop")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_connected_time_ms_key)
    .andReturnValue(0);
  // started
  memfault_metrics_connectivity_connected_state_change(
    kMemfaultMetricsConnectivityState_ConnectionLost);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(kMemfaultMetricsConnectivityState_Started);
  // connected
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_expected_time_ms_key)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_timer_start")
    .withParameterOfType("MemfaultMetricId", "key", &connectivity_connected_time_ms_key)
    .andReturnValue(0);
  memfault_metrics_connectivity_connected_state_change(kMemfaultMetricsConnectivityState_Connected);
}
