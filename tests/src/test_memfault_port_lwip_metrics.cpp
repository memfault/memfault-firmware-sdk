#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "lwip/stats.h"
#include "memfault/ports/lwip/metrics.h"

static MemfaultMetricIdsComparator s_metric_id_comparator;
struct mock_stats lwip_stats = {0};

static MemfaultMetricId s_tcp_tx_count_id = MEMFAULT_METRICS_KEY(tcp_tx_count);
static MemfaultMetricId s_tcp_rx_count_id = MEMFAULT_METRICS_KEY(tcp_rx_count);
static MemfaultMetricId s_tcp_drop_count_id = MEMFAULT_METRICS_KEY(tcp_drop_count);
static MemfaultMetricId s_udp_tx_count_id = MEMFAULT_METRICS_KEY(udp_tx_count);
static MemfaultMetricId s_udp_rx_count_id = MEMFAULT_METRICS_KEY(udp_rx_count);
static MemfaultMetricId s_udp_drop_count_id = MEMFAULT_METRICS_KEY(udp_drop_count);

static void prv_set_metrics(unsigned int value) {
  lwip_stats.tcp.xmit = value;
  lwip_stats.tcp.recv = value;
  lwip_stats.tcp.drop = value;
  lwip_stats.udp.xmit = value;
  lwip_stats.udp.recv = value;
  lwip_stats.udp.drop = value;
}

static void prv_clear_metrics(void) {
  // Clear the stats global
  memset(&lwip_stats, 0, sizeof lwip_stats);

  // Clear the static delta values, this requires collecting 2 rounds of metrics
  memfault_lwip_heartbeat_collect_data();
  memfault_lwip_heartbeat_collect_data();
}

static void prv_set_expected_calls(unsigned int expected_value) {
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_tcp_tx_count_id)
    .withParameter("unsigned_value", expected_value);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_tcp_rx_count_id)
    .withParameter("unsigned_value", expected_value);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_tcp_drop_count_id)
    .withParameter("unsigned_value", expected_value);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_udp_tx_count_id)
    .withParameter("unsigned_value", expected_value);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_udp_rx_count_id)
    .withParameter("unsigned_value", expected_value);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_udp_drop_count_id)
    .withParameter("unsigned_value", expected_value);
}

// clang-format off
TEST_GROUP(LwipMetrics) {
  void setup(){
    // Disable mock framework while clearing metrics
    mock().disable();
    prv_clear_metrics();
    mock().enable();

    mock().strictOrder();
    mock().installComparator("MemfaultMetricId", s_metric_id_comparator);
  }
  void teardown(){
    mock().removeAllComparatorsAndCopiers();
    mock().clear();
  }
};
// clang-format on

TEST(LwipMetrics, MetricUpdate) {
  // Test with an initial value update
  unsigned int new_value = 128;
  prv_set_metrics(new_value);
  prv_set_expected_calls(new_value);
  memfault_lwip_heartbeat_collect_data();

  // Check delta is produced (values are 0)
  prv_set_expected_calls(0);
  memfault_lwip_heartbeat_collect_data();

  // Set values again and check values are set to delta
  prv_set_metrics(new_value * 2);
  prv_set_expected_calls(new_value);
  memfault_lwip_heartbeat_collect_data();
};

TEST(LwipMetrics, MetricRollover) {
  unsigned int delta = 256;
  // Set the initial value to close to the uint32_t max
  // Use half of the delta to easily check for wrap around behavior later
  unsigned int initial_value = UINT32_MAX - (delta >> 1);

  // Initialize the stats and check for correct metric values
  prv_set_metrics(initial_value);
  prv_set_expected_calls(initial_value);
  memfault_lwip_heartbeat_collect_data();

  // Update the new value to UINT32_MAX + delta to force a wraparound
  unsigned int new_value = initial_value + delta;
  prv_set_metrics(new_value);
  // Metric values should be equal to the delta and handle wraparound correctly
  prv_set_expected_calls(delta);
  memfault_lwip_heartbeat_collect_data();
};
