#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "components/core/src/memfault_self_test_private.h"
#include "memfault/core/math.h"
#include "memfault/core/self_test.h"
#include "memfault/panics/coredump.h"
#include "mocks/mock_memfault_platform_debug_log.h"

bool memfault_coredump_storage_debug_test_begin(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_coredump_storage_debug_test_finish(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_self_test_platform_enable_irqs(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_self_test_platform_disable_irqs(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_coredump_storage_check_size(void) {
  return mock().actualCall(__func__).returnBoolValue();
}

bool memfault_coredump_has_valid_coredump(size_t *total_size_out) {
  return mock()
    .actualCall(__func__)
    .withOutputParameter("total_size_out", total_size_out)
    .returnBoolValue();
}

static void prv_setup_irq_mocks(void) {
  mock().expectOneCall("memfault_self_test_platform_disable_irqs").andReturnValue(true);
  mock().expectOneCall("memfault_self_test_platform_enable_irqs").andReturnValue(true);
}

TEST_GROUP(MemfaultSelfTestCoredumpStorage) {
  void setup() {
    const char *output_lines[] = {
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      "Coredump Storage Test",
      MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
      MEMFAULT_SELF_TEST_END_OUTPUT,
    };
    memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                   MEMFAULT_ARRAY_SIZE(output_lines));
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestCoredumpStorage, Test_HasValidCoredump) {
  mock()
    .expectOneCall("memfault_coredump_has_valid_coredump")
    .withUnmodifiedOutputParameter("total_size_out")
    .andReturnValue(true);

  const char *error_lines[] = {
    "Aborting test, valid coredump present",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  uint32_t result = memfault_self_test_coredump_storage_test();
  UNSIGNED_LONGS_EQUAL((1 << 0), result);
}

TEST(MemfaultSelfTestCoredumpStorage, Test_DisableIrqFailure) {
  mock()
    .expectOneCall("memfault_coredump_has_valid_coredump")
    .withUnmodifiedOutputParameter("total_size_out")
    .andReturnValue(false);
  mock().expectOneCall("memfault_self_test_platform_disable_irqs").andReturnValue(false);

  const char *error_lines[] = {
    "Aborting test, could not disable interrupts",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Error, error_lines,
                                 MEMFAULT_ARRAY_SIZE(error_lines));

  uint32_t result = memfault_self_test_coredump_storage_test();
  UNSIGNED_LONGS_EQUAL((1 << 1), result);
}

TEST(MemfaultSelfTestCoredumpStorage, Test_EnableIrqFailure) {
  mock()
    .expectOneCall("memfault_coredump_has_valid_coredump")
    .withUnmodifiedOutputParameter("total_size_out")
    .andReturnValue(false);
  mock().expectOneCall("memfault_self_test_platform_disable_irqs").andReturnValue(true);
  mock().expectOneCall("memfault_coredump_storage_debug_test_begin").andReturnValue(true);
  mock().expectOneCall("memfault_self_test_platform_enable_irqs").andReturnValue(false);
  mock().expectOneCall("memfault_coredump_storage_debug_test_finish").andReturnValue(true);

  const char *warning_lines[] = {
    "Failed to enable interrupts after test completed",
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Warning, warning_lines,
                                 MEMFAULT_ARRAY_SIZE(warning_lines));

  uint32_t result = memfault_self_test_coredump_storage_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}

TEST(MemfaultSelfTestCoredumpStorage, Test_CoredumpStorage_Failure) {
  prv_setup_irq_mocks();
  mock()
    .expectOneCall("memfault_coredump_has_valid_coredump")
    .withUnmodifiedOutputParameter("total_size_out")
    .andReturnValue(false);
  mock().expectOneCall("memfault_coredump_storage_debug_test_begin").andReturnValue(false);
  mock().expectOneCall("memfault_coredump_storage_debug_test_finish").andReturnValue(false);

  uint32_t result = memfault_self_test_coredump_storage_test();
  UNSIGNED_LONGS_EQUAL((1 << 2), result);
}

TEST(MemfaultSelfTestCoredumpStorage, Test_CoredumpStorage_Success) {
  prv_setup_irq_mocks();
  mock()
    .expectOneCall("memfault_coredump_has_valid_coredump")
    .withUnmodifiedOutputParameter("total_size_out")
    .andReturnValue(false);
  mock().expectOneCall("memfault_coredump_storage_debug_test_begin").andReturnValue(true);
  mock().expectOneCall("memfault_coredump_storage_debug_test_finish").andReturnValue(true);

  uint32_t result = memfault_self_test_coredump_storage_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}

TEST_GROUP(MemfaultSelfTestCoredumpStorageCapacity) {
  void setup() { }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultSelfTestCoredumpStorageCapacity, Test_CoredumpStorageCapacity_TooSmall) {
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    "Coredump Storage Capacity Test",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT,
    MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_coredump_storage_check_size").andReturnValue(false);

  uint32_t result = memfault_self_test_coredump_storage_capacity_test();
  UNSIGNED_LONGS_EQUAL(1, result);
}

TEST(MemfaultSelfTestCoredumpStorageCapacity, Test_CoredumpStorageCapacity_Success) {
  const char *output_lines[] = {
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, "Coredump Storage Capacity Test",
    "Total size required: 0 bytes",  "Storage capacity: 0 bytes",
    MEMFAULT_SELF_TEST_BEGIN_OUTPUT, MEMFAULT_SELF_TEST_END_OUTPUT,
  };
  memfault_platform_log_set_mock(kMemfaultPlatformLogLevel_Info, output_lines,
                                 MEMFAULT_ARRAY_SIZE(output_lines));

  mock().expectOneCall("memfault_coredump_storage_check_size").andReturnValue(true);
  uint32_t result = memfault_self_test_coredump_storage_capacity_test();
  UNSIGNED_LONGS_EQUAL(0, result);
}
