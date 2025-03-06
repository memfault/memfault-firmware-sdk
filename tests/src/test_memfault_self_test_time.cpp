#include <chrono>
#include <sstream>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "components/core/src/memfault_self_test_private.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/system_time.h"
#include "memfault/core/self_test.h"
#include "mocks/mock_memfault_platform_debug_log.h"

extern "C" {
uint64_t memfault_platform_get_time_since_boot_ms(void) {
  return mock().actualCall(__func__).returnUnsignedLongIntValue();
}

void memfault_self_test_platform_delay(MEMFAULT_UNUSED uint32_t delay_ms) {
  return;
}
}

// Initializes a valid timestamp using the current system time
static void prv_initialize_valid_time(sMemfaultCurrentTime *time) {
  // Get a timepoint relative to epoch
  const auto since_epoch = std::chrono::system_clock::now().time_since_epoch();
  // Convert duration to seconds then resolve to an integer with count
  const auto unix_timestamp_secs =
    std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();

  // Populate time struct
  *time = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = (uint64_t)unix_timestamp_secs,
    },
  };
}

// Helper function to set up valid memfault_platform_time_get_current results
// Parameter time MUST remain in scope until test completes
static void prv_set_time_get_current_mock(sMemfaultCurrentTime *time, bool return_value) {
  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", time, sizeof(sMemfaultCurrentTime))
    .andReturnValue(return_value);
}

// Helper function to set up valid memfault_platform_get_time_since_boot_ms
static void prv_set_valid_time_since_boot(void) {
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(1);
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(2);
}

// Helper function to create a string containing expected output formatted with the current time
// Caller of the function must delete the return value
// NB: Attempting to return a string object causes a memory leak to occur at the end of each test.
// The easiest remedy appears to be to use a heap allocated object instead
static std::string *prv_build_timestamp_received_string(sMemfaultCurrentTime *time) {
  std::stringstream ss;
  ss << "Verify received timestamp for accuracy. Timestamp received "
     << (uint64_t)time->info.unix_timestamp_secs;
  return new std::string(ss.str());
}

// Stores the current time struct used by a test mock. Static storage is used to ensure the struct
// is around when the mock calls are checked
static sMemfaultCurrentTime s_current_time;
// Stores a pointer to the current expected output string formed from the current time. Static
// storage is used to ensure the struct is around when the mock calls are checked
static std::string *s_timestamp_received_str;

TEST_GROUP(MemfaultSelfTestTime_GetTimeSinceBootMs) {
  void setup() {
    // Initialize time_get_current to valid output
    prv_initialize_valid_time(&s_current_time);
    prv_set_time_get_current_mock(&s_current_time, true);

    // Build up the output string using the current time
    s_timestamp_received_str = prv_build_timestamp_received_string(&s_current_time);
    // clang-format off
    const char * info_lines[] = {
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      "Time Test",
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      s_timestamp_received_str->c_str(),
      MEMFAULT_SELF_TEST_END_OUTPUT,
    };
    // clang-format on

    memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, info_lines,
                                   MEMFAULT_ARRAY_SIZE(info_lines));
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
    // Delete the string returns from prv_build_timestamp_received_string
    delete s_timestamp_received_str;
  }
};

TEST(MemfaultSelfTestTime_GetTimeSinceBootMs, Test_ZeroBootTime) {

  // Initialize logging mock with expected error lines and number of additional calls
  const char *error_lines[] = {
    "Time since boot reported as 0",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(0);

  // Initialize memfault_platform_time_get_current
  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 0), result);
}

TEST(MemfaultSelfTestTime_GetTimeSinceBootMs, Test_EndEqualStart) {
  // Run test with the same start and end timestamps
  // Set up logging mock with expected error lines
  const char *error_lines[] = {
    "Time since boot not monotonically increasing: start[4] vs end[4]",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  // Set up mock to return the same time since boot value for both calls
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(4);
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(4);

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 1), result);
}

TEST(MemfaultSelfTestTime_GetTimeSinceBootMs, Test_EndLessThanStart) {
  // Run test with start timestamp > end timestamp
  const char *error_lines[] = {
    "Time since boot not monotonically increasing: start[4] vs "
    "end[3]",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  // Set up mock to return start > end
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(4);
  mock().expectOneCall("memfault_platform_get_time_since_boot_ms").andReturnValue(3);

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 1), result);
}

TEST_GROUP(MemfaultSelfTestTime_TimeGetCurrent) {
  void setup() {
    // Initialize boot time
    prv_set_valid_time_since_boot();

    // clang-format off
    const char * info_lines[] = {
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      "Time Test",
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      "Time since boot test succeeded",
      MEMFAULT_SELF_TEST_END_OUTPUT,
    };
    // clang-format on

    memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, info_lines,
                                   MEMFAULT_ARRAY_SIZE(info_lines));
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestTime_TimeGetCurrent, Test_Timestamp_Unavailable) {
  const char *error_lines[] = {
    "Current timestamp could not be recovered",
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  sMemfaultCurrentTime time = {};
  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", &time, sizeof(time))
    .andReturnValue(false);

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 2), result);
}

TEST(MemfaultSelfTestTime_TimeGetCurrent, Test_WrongType) {
  sMemfaultCurrentTime time = {
    .type = kMemfaultCurrentTimeType_Unknown,
  };
  const char *error_lines[] = {
    "Invalid time type returned: 0",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", &time, sizeof(time))
    .andReturnValue(true);

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 3), result);
}

TEST(MemfaultSelfTestTime_TimeGetCurrent, Test_EpochTimeResult) {
  sMemfaultCurrentTime time = {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 0,
    },
  };

  const char *error_lines[] = {
    "Timestamp too far in the past: 0",
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  mock()
    .expectOneCall("memfault_platform_time_get_current")
    .withOutputParameterReturning("time", &time, sizeof(time))
    .andReturnValue(true);

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL((1 << 4), result);
}

TEST(MemfaultSelfTestTime_TimeGetCurrent, Test_HappyPath) {
  sMemfaultCurrentTime time = {};
  prv_initialize_valid_time(&time);
  prv_set_time_get_current_mock(&time, true);
  std::string *timestamp_received = prv_build_timestamp_received_string(&time);

  const char *info_lines[] = {
    timestamp_received->c_str(),
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, info_lines,
                                 MEMFAULT_ARRAY_SIZE(info_lines));

  uint32_t result = memfault_self_test_time_test();
  UNSIGNED_LONGS_EQUAL(0, result);

  delete timestamp_received;
}
