//! @file
//!
//! @brief

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_memfault_fault_handling.hpp"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/task_watchdog.h"

extern "C" {
static uint64_t s_fake_time_ms = 0;
uint64_t memfault_platform_get_time_since_boot_ms(void) { return s_fake_time_ms; }
}

void memfault_task_watchdog_platform_refresh_callback(void) { mock().actualCall(__func__); }

static Mflt_sMemfaultAssertInfo_Comparator s_assert_info_comparator;

TEST_GROUP(MemfaultTaskWatchdog){void setup(){mock().strictOrder();
mock().installComparator("sMemfaultAssertInfo", s_assert_info_comparator);

s_fake_time_ms = 0;

memfault_task_watchdog_init();
}
void teardown() {
  mock().checkExpectations();
  mock().removeAllComparatorsAndCopiers();
  mock().clear();
}
}
;

TEST(MemfaultTaskWatchdog, Test_Basic) {
  // no channels started
  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();

  // start a channel, confirm we don't expire when time hasn't advanced
  MEMFAULT_TASK_WATCHDOG_START(task_1);
  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();

  // call for coverage
  memfault_task_watchdog_bookkeep();
}

TEST(MemfaultTaskWatchdog, Test_Expire) {
  // start a channel and artificially advance time to expire it
  MEMFAULT_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS - 1;
  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();

  s_fake_time_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;
  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();

  s_fake_time_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;

  sMemfaultAssertInfo extra_info = {
    .assert_reason = kMfltRebootReason_SoftwareWatchdog,
  };
  mock()
    .expectOneCall("memfault_fault_handling_assert_extra")
    .withPointerParameter("pc", 0)
    .withPointerParameter("lr", 0)
    .withParameterOfType("sMemfaultAssertInfo", "extra_info", &extra_info);

  memfault_task_watchdog_check_all();
}

TEST(MemfaultTaskWatchdog, Test_Feed) {
  // start a channel, advance time past the timeout, feed it, confirm it doesn't
  // expire
  MEMFAULT_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;
  MEMFAULT_TASK_WATCHDOG_FEED(task_1);

  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();
}

TEST(MemfaultTaskWatchdog, Test_Stop) {
  // start a channel, advance time past the timeout, feed it, confirm it doesn't
  // expire
  MEMFAULT_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;
  MEMFAULT_TASK_WATCHDOG_STOP(task_1);

  mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
  memfault_task_watchdog_check_all();
}

TEST(MemfaultTaskWatchdog, Test_ExpireWrapAround) {
  // start a channel and artificially advance time to expire it. check
  // the wraparound cases.
  const uint64_t wrap_start_points[] = {
    UINT32_MAX,
    UINT64_MAX,
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(wrap_start_points); i++){
    s_fake_time_ms = wrap_start_points[i] - MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;
    MEMFAULT_TASK_WATCHDOG_START(task_1);

    s_fake_time_ms += MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS - 1;
    mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
    memfault_task_watchdog_check_all();

    s_fake_time_ms += 1;
    mock().expectOneCall("memfault_task_watchdog_platform_refresh_callback");
    memfault_task_watchdog_check_all();

    s_fake_time_ms += 1;

    sMemfaultAssertInfo extra_info = {
      .assert_reason = kMfltRebootReason_SoftwareWatchdog,
    };
    mock()
      .expectOneCall("memfault_fault_handling_assert_extra")
      .withPointerParameter("pc", 0)
      .withPointerParameter("lr", 0)
      .withParameterOfType("sMemfaultAssertInfo", "extra_info", &extra_info);

    memfault_task_watchdog_check_all();
  }
}
