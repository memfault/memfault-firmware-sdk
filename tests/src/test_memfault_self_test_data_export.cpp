#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "memfault/core/math.h"
#include "memfault_self_test_private.h"
#include "mocks/mock_memfault_platform_debug_log.h"

TEST_GROUP(MemfaultSelfTestDeviceInfo) {
  void setup() {}
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestDeviceInfo, Test_DataExportTest) {
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Data Export Line Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    // Should be equal to MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN - 1 chars
    "Printing 112 characters, confirm line ends with '1' and is not split",
    // Parentheses needed for -Wstring-concatenation
    ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
     "++++++++++++++++++1"),
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  memfault_self_test_data_export_test();
}
