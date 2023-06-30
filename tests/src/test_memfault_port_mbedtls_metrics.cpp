#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_metric_ids.hpp"
#include "mbedtls_mem.h"
#include "memfault/core/math.h"
#include "memfault/ports/mbedtls/metrics.h"
#include "memfault/util/align.h"

static MemfaultMetricIdsComparator s_metric_id_comparator;

static MemfaultMetricId s_mbedtls_mem_used_id = MEMFAULT_METRICS_KEY(mbedtls_mem_used_bytes);
static MemfaultMetricId s_mbedtls_mem_max_id = MEMFAULT_METRICS_KEY(mbedtls_mem_max_bytes);

static void prv_set_expected_calls(int expected_used, unsigned int expected_max) {
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_signed")
    .withParameterOfType("MemfaultMetricId", "key", &s_mbedtls_mem_used_id)
    .withParameter("signed_value", expected_used);
  mock()
    .expectOneCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &s_mbedtls_mem_max_id)
    .withParameter("unsigned_value", expected_max);
}

static void *prv_test_calloc(size_t num, size_t size) {
  void *ptr = __wrap_mbedtls_calloc(num, size);
  // Ensure that memory is max aligned
  CHECK_EQUAL(0, (uintptr_t)ptr % MEMFAULT_MAX_ALIGN_SIZE);
  return ptr;
}

// clang-format off
TEST_GROUP(MbedtlsMetrics) {
  void setup(){
    mock().strictOrder();
    mock().installComparator("MemfaultMetricId", s_metric_id_comparator);
  }
  void teardown(){
    memfault_mbedtls_test_clear_values();
    mock().removeAllComparatorsAndCopiers();
    mock().clear();
  }
};
// clang-format on

TEST(MbedtlsMetrics, MetricUpdate) {
  // Test metric values after calloc
  size_t num = 10;
  size_t size = 5;

  prv_set_expected_calls(num * size, num * size);
  void *ptr_a = prv_test_calloc(num, size);
  memfault_mbedtls_heartbeat_collect_data();

  // Test metric values after free
  __wrap_mbedtls_free(ptr_a);
  prv_set_expected_calls(0, num * size);
  memfault_mbedtls_heartbeat_collect_data();

  // Test metric values after mix of calloc/free
  prv_set_expected_calls(2 * num * size, 2 * num * size);
  prv_set_expected_calls(num * size, 2 * num * size);
  prv_set_expected_calls(0, 2 * num * size);

  void *ptr_b = prv_test_calloc(num, size);
  ptr_a = prv_test_calloc(num, size);

  memfault_mbedtls_heartbeat_collect_data();
  __wrap_mbedtls_free(ptr_a);
  memfault_mbedtls_heartbeat_collect_data();
  __wrap_mbedtls_free(ptr_b);
  memfault_mbedtls_heartbeat_collect_data();
};

TEST(MbedtlsMetrics, CallocAlignment) {
  // Iterate through a wide range of sizes to ensure returned memory is aligned
  void *ptr_array[MEMFAULT_MAX_ALIGN_SIZE];
  for (unsigned int i = 0; i < MEMFAULT_ARRAY_SIZE(ptr_array); i++) {
    ptr_array[i] = prv_test_calloc(1, i);
  }

  for (unsigned int i = 0; i < MEMFAULT_ARRAY_SIZE(ptr_array); i++) {
    __wrap_mbedtls_free(ptr_array[i]);
  }
};
