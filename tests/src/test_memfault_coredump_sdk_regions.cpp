//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "memfault/core/log_impl.h"
#include "memfault/panics/coredump_impl.h"

TEST_GROUP(MemfaultSdkRegions){
  void setup() {
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

bool memfault_log_get_regions(sMemfaultLogRegions *regions) {
  const bool enabled = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (!enabled) {
    return false;
  }

  for (size_t i = 0; i < MEMFAULT_LOG_NUM_RAM_REGIONS; i++) {
    sMemfaultLogMemoryRegion *region = &regions->region[i];
    region->region_start = (void *)i;
    region->region_size = i + 1;
  }
  return true;
}

TEST(MemfaultSdkRegions, Test_MemfaultSdkRegions_Enabled) {

  mock().expectOneCall("memfault_log_get_regions");
  size_t num_regions;
  const sMfltCoredumpRegion *regions = memfault_coredump_get_sdk_regions(&num_regions);
  LONGS_EQUAL(2, num_regions);

  for (size_t i = 0; i < MEMFAULT_LOG_NUM_RAM_REGIONS; i++) {
    const sMfltCoredumpRegion *region = &regions[i];
    LONGS_EQUAL(kMfltCoredumpRegionType_Memory, region->type);
    LONGS_EQUAL(i, (uint32_t)(uintptr_t)region->region_start);
    LONGS_EQUAL(i + 1, (uint32_t)region->region_size);
  }
}

TEST(MemfaultSdkRegions, Test_MemfaultSdkRegions_Disabled) {
  mock().expectOneCall("memfault_log_get_regions").andReturnValue(false);
  size_t num_regions;
  const sMfltCoredumpRegion *regions = memfault_coredump_get_sdk_regions(&num_regions);
  LONGS_EQUAL(0, num_regions);
  CHECK(regions == NULL);
}
