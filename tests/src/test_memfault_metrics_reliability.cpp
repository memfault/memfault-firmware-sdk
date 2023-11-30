

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/reliability.h"

static MemfaultMetricIdsComparator s_metric_id_comparator;

// These have to have global scope, so the test teardown can access them
static MemfaultMetricId operational_hours_key = MEMFAULT_METRICS_KEY(operational_hours);
static MemfaultMetricId operational_crashfree_hours_key =
  MEMFAULT_METRICS_KEY(operational_crashfree_hours);

extern "C" {
static uint64_t s_fake_time_ms = 0;
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return s_fake_time_ms;
}

static void prv_fake_time_incr(uint64_t fake_time_delta_ms) {
  s_fake_time_ms += fake_time_delta_ms;
}
}

// clang-format off
TEST_GROUP(MemfaultMetricsReliability){
  void setup() {
    s_fake_time_ms = 0;
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

TEST(MemfaultMetricsReliability, Test_OperationalHours) {
  // 1 ms less than 1 hr
  prv_fake_time_incr(60 * 60 * 1000 - 1);
  // no mocks should be called
  memfault_metrics_reliability_collect();

  // 1 ms more (1 hr total)
  prv_fake_time_incr(1);
  bool unexpected_reboot = true;
  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &operational_hours_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &operational_crashfree_hours_key)
    .withParameter("amount", 0)
    .andReturnValue(0);

  memfault_metrics_reliability_collect();

  // 1 hr more
  prv_fake_time_incr(60 * 60 * 1000);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &operational_hours_key)
    .withParameter("amount", 1)
    .andReturnValue(0);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_add")
    .withParameterOfType("MemfaultMetricId", "key", &operational_crashfree_hours_key)
    .withParameter("amount", 1)
    .andReturnValue(0);

  memfault_metrics_reliability_collect();

  // 1 ms more (2hr 1ms total)
  prv_fake_time_incr(1);
  // no mocks should be called
  memfault_metrics_reliability_collect();
}
