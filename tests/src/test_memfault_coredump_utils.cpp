//! @file
//!

#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "memfault/panics/coredump.h"

static sMfltCoredumpStorageInfo s_fake_coredump_info;

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = s_fake_coredump_info;
}

size_t memfault_coredump_storage_compute_size_required(void) {
  return 10;
}


TEST_GROUP(MfltCoredumpUtilTestGroup) {
  void setup() {
  }

  void teardown() {
    memset(&s_fake_coredump_info, 0x0, sizeof(s_fake_coredump_info));
  }
};


TEST(MfltCoredumpUtilTestGroup, Test_MfltCoredumpUtilSizeCheck) {
  bool check_passed = memfault_coredump_storage_check_size();
  CHECK(!check_passed);

  s_fake_coredump_info.size = memfault_coredump_storage_compute_size_required() - 1;
  check_passed = memfault_coredump_storage_check_size();
  CHECK(!check_passed);

  s_fake_coredump_info.size++;
  check_passed = memfault_coredump_storage_check_size();
  CHECK(check_passed);
}
