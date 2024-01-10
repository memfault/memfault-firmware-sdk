#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "components/core/src/memfault_self_test_private.h"
#include "memfault/core/math.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"
#include "memfault/panics/platform/coredump.h"
#include "mocks/mock_memfault_platform_debug_log.h"

extern "C" {

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  return (const sMfltCoredumpRegion *)mock()
    .actualCall(__func__)
    .withOutputParameter("num_regions", (void *)num_regions)
    .returnConstPointerValue();
}

const sMfltCoredumpRegion *memfault_coredump_get_sdk_regions(size_t *num_regions) {
  return (const sMfltCoredumpRegion *)mock()
    .actualCall(__func__)
    .withOutputParameter("num_regions", num_regions)
    .returnConstPointerValue();
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  MEMFAULT_UNUSED const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  return (const sMfltCoredumpRegion *)mock()
    .actualCall(__func__)
    .withOutputParameter("num_regions", (void *)num_regions)
    .returnConstPointerValue();
}
}

typedef struct {
  const sMfltCoredumpRegion *platform_regions;
  size_t num_platform_regions;
  const sMfltCoredumpRegion *arch_regions;
  size_t num_arch_regions;
  const sMfltCoredumpRegion *sdk_regions;
  size_t num_sdk_regions;
} sTestParameters;

// Helper function to setup the coredump region mocks
static void prv_set_coredump_region_mocks(sTestParameters *params) {
  mock()
    .expectOneCall("memfault_platform_coredump_get_regions")
    .withOutputParameterReturning("num_regions", (void *)&params->num_platform_regions,
                                  sizeof(params->num_platform_regions))
    .andReturnValue((const void *)params->platform_regions);

  mock()
    .expectOneCall("memfault_coredump_get_arch_regions")
    .withOutputParameterReturning("num_regions", (void *)&params->num_arch_regions,
                                  sizeof(params->num_arch_regions))
    .andReturnValue((const void *)params->arch_regions);

  mock()
    .expectOneCall("memfault_coredump_get_sdk_regions")
    .withOutputParameterReturning("num_regions", (void *)&params->num_sdk_regions,
                                  sizeof(params->num_sdk_regions))
    .andReturnValue((const void *)params->sdk_regions);
}

TEST_GROUP(MemfaultSelfTestCoredumpRegions) {
  void setup() {}
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestCoredumpRegions, Test_NoRegions) {
  sTestParameters params = { 0 };

  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump Regions Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump group: Platform Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "-----------------------------",
    "Coredump group: Arch Regions",
    "-----------------------------",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "Coredump group: SDK Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  // clang-format on

  const char *error_lines[] = {
    "Number of platform regions = 0",
    "Platform regions was NULL",
  };

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));
  prv_set_coredump_region_mocks(&params);

  uint32_t result = memfault_self_test_coredump_regions_test();
  UNSIGNED_LONGS_EQUAL(1, result);
}

TEST(MemfaultSelfTestCoredumpRegions, Test_PlatformRegionsOutput) {
  const sMfltCoredumpRegion test_regions[2] = {
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0x0,
      .region_size = 50000,
    },
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0xFFFF,
      .region_size = 1000,
    },
  };
  sTestParameters params = {
    .platform_regions = (const sMfltCoredumpRegion *)&test_regions,
    .num_platform_regions = MEMFAULT_ARRAY_SIZE(test_regions),
  };

  prv_set_coredump_region_mocks(&params);

  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump Regions Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump group: Platform Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "0x00000000|     50000|     1|",
    "0x0000ffff|      1000|     1|",
    "-----------------------------",
    "-----------------------------",
    "Coredump group: Arch Regions",
    "-----------------------------",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "Coredump group: SDK Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  uint32_t result = memfault_self_test_coredump_regions_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}

TEST(MemfaultSelfTestCoredumpRegions, Test_ArchRegionsOutput) {
  const sMfltCoredumpRegion test_regions[2] = {
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0x0,
      .region_size = 50000,
    },
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0xFFFF,
      .region_size = 1000,
    },
  };
  sTestParameters params = {
    .platform_regions = (const sMfltCoredumpRegion *)&test_regions,
    .num_platform_regions = MEMFAULT_ARRAY_SIZE(test_regions),
    .arch_regions = (const sMfltCoredumpRegion *)&test_regions,
    .num_arch_regions = MEMFAULT_ARRAY_SIZE(test_regions),
  };

  prv_set_coredump_region_mocks(&params);

  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump Regions Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump group: Platform Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "0x00000000|     50000|     1|",
    "0x0000ffff|      1000|     1|",
    "-----------------------------",
    "-----------------------------",
    "Coredump group: Arch Regions",
    "-----------------------------",
    "0x00000000|     50000|     1|",
    "0x0000ffff|      1000|     1|",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "Coredump group: SDK Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  uint32_t result = memfault_self_test_coredump_regions_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}

TEST(MemfaultSelfTestCoredumpRegions, Test_SdkRegionsOutput) {
  const sMfltCoredumpRegion test_regions[2] = {
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0x0,
      .region_size = 50000,
    },
    {
      .type = (eMfltCoredumpRegionType)1,
      .region_start = (void *)0xFFFF,
      .region_size = 1000,
    },
  };
  sTestParameters params = {
    .platform_regions = (const sMfltCoredumpRegion *)&test_regions,
    .num_platform_regions = MEMFAULT_ARRAY_SIZE(test_regions),
    .sdk_regions = (const sMfltCoredumpRegion *)&test_regions,
    .num_sdk_regions = MEMFAULT_ARRAY_SIZE(test_regions),
  };

  prv_set_coredump_region_mocks(&params);

  // clang-format off
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump Regions Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump group: Platform Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "0x00000000|     50000|     1|",
    "0x0000ffff|      1000|     1|",
    "-----------------------------",
    "-----------------------------",
    "Coredump group: Arch Regions",
    "-----------------------------",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "Coredump group: SDK Regions",
    "-----------------------------",
    "   Address|    Length|  Type|",
    "-----------------------------",
    "0x00000000|     50000|     1|",
    "0x0000ffff|      1000|     1|",
    "-----------------------------",
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  // clang-format on

  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  uint32_t result = memfault_self_test_coredump_regions_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}
