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
#include "memfault/components.h"
#include "memfault/metrics/serializer.h"

extern "C" {
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return 0;
}
}

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MEMFAULT_UNUSED MemfaultPlatformTimerCallback callback) {
  return mock()
    .actualCall(__func__)
    .withParameter("period_sec", period_sec)
    .returnBoolValueOrDefault(true);
}

size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(0);
}

bool memfault_metrics_heartbeat_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

bool memfault_metrics_session_serialize(
  MEMFAULT_UNUSED const sMemfaultEventStorageImpl *storage_impl,
  MEMFAULT_UNUSED eMfltMetricsSessionIndex session_index) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

// clang-format off
TEST_GROUP(MemfaultHeartbeatMetricsNoCustom){
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};
// clang-format on

//! Confirm compilation and metric count is correct
TEST(MemfaultHeartbeatMetricsNoCustom, Test_CompileAndMetricCount) {
  size_t num_metrics = memfault_metrics_heartbeat_get_num_metrics();
  LONGS_EQUAL(4, num_metrics);
}

//! Confirm we can boot without any issues (eg writing to unpopulated key
//! position)
TEST(MemfaultHeartbeatMetricsNoCustom, Test_Boot) {
  // Check that by default the heartbeat interval is once / hour
  mock().expectOneCall("memfault_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  mock()
    .expectOneCall("memfault_metrics_heartbeat_compute_worst_case_storage_size")
    .andReturnValue(0);

  static uint8_t s_storage[1000];

  static const sMemfaultEventStorageImpl *fake_event_storage_impl =
    memfault_events_storage_boot(&s_storage, sizeof(s_storage));

  sMemfaultMetricBootInfo boot_info = {.unexpected_reboot_count = 1};
  int rv = memfault_metrics_boot(fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(0, rv);

  // call this to get coverage on the internal weak function
  memfault_metrics_heartbeat_collect_data();
}
