#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

extern "C" {
  #include "fakes/fake_memfault_platform_coredump_storage.h"
  #include "memfault/core/compiler.h"
  #include "memfault/core/platform/core.h"
  #include "memfault/core/platform/device_info.h"
  #include "memfault/panics/coredump.h"
  #include "memfault/panics/coredump_impl.h"
  #include "memfault/panics/platform/coredump.h"

  static uint8_t s_storage_buf[4 * 1024] MEMFAULT_ALIGNED(0x8);

  static sMfltCoredumpStorageInfo s_fake_storage_info;
  static sMfltCoredumpRegion s_fake_memory_region[2];
  static size_t s_num_fake_regions;

  static sMfltCoredumpRegion s_fake_arch_region[2];
  static size_t s_num_fake_arch_regions;

  void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
    *info = (struct MemfaultDeviceInfo) {
      .device_serial = "1",
      .software_type = "main",
      .software_version = "22",
      .hardware_version = "333",
    };
  }
}

void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  *info = s_fake_storage_info;
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(size_t *num_regions) {
  *num_regions = s_num_fake_regions;
  return &s_fake_memory_region[0];
}

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  *num_regions = s_num_fake_arch_regions;
  return &s_fake_arch_region[0];
}

TEST_GROUP(MfltCoredumpTestGroup) {
  void setup() {
    fake_memfault_platform_coredump_storage_setup(s_storage_buf, sizeof(s_storage_buf));
    memfault_platform_coredump_storage_erase(0, sizeof(s_storage_buf));
    s_fake_storage_info.size = sizeof(s_storage_buf);
    s_fake_storage_info.sector_size = 256; // Not really used yet

    static uint8_t s_fake_core_region0[] = { 'h', 'e', 'l', 'l', 'o','!','!','!' };
    static uint8_t s_fake_core_region1[] = { 'a', 'b', 'c', 'd' };
    s_fake_memory_region[0].type = kMfltCoredumpRegionType_Memory;
    s_fake_memory_region[0].region_start = &s_fake_core_region0;
    s_fake_memory_region[0].region_size = sizeof(s_fake_core_region0);

    s_fake_memory_region[1].type = kMfltCoredumpRegionType_Memory;
    s_fake_memory_region[1].region_start = &s_fake_core_region1;
    s_fake_memory_region[1].region_size = sizeof(s_fake_core_region1);

    s_num_fake_regions = 2;

    static uint8_t s_fake_arch_region1[] = { 'e', 'f', 'g' };
    s_fake_arch_region[0].type = kMfltCoredumpRegionType_Memory;
    s_fake_arch_region[0].region_start = &s_fake_arch_region1;
    s_fake_arch_region[0].region_size = sizeof(s_fake_arch_region1);

    s_num_fake_arch_regions = 1;
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_assert_storage_empty(void) {
  size_t total_coredump_size;
  bool has_coredump = memfault_coredump_has_valid_coredump(&total_coredump_size);
  CHECK(!has_coredump);

  uint8_t empty_storage[sizeof(s_storage_buf)];
  memset(empty_storage, 0xff, sizeof(empty_storage));
  MEMCMP_EQUAL(empty_storage, s_storage_buf, sizeof(s_storage_buf));
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpNoRegions) {
  s_num_fake_regions = 0;
  s_num_fake_arch_regions = 0;
  memfault_coredump_save(NULL, 0, 0);

  prv_assert_storage_empty();
}

// Test the basics ... make sure the coredump is flushed out in the order we expect
TEST(MfltCoredumpTestGroup, Test_MfltCoredumpSaveCore) {
  const uint32_t regs[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };
  const uint32_t trace_reason = 0xdead;

  memfault_coredump_save((void *)&regs, sizeof(regs), trace_reason);

  uint8_t *coredump_buf = &s_storage_buf[0];
  const uint32_t expected_header_magic = 0x45524f43;
  MEMCMP_EQUAL(coredump_buf, &expected_header_magic, sizeof(expected_header_magic));
  coredump_buf += sizeof(expected_header_magic);
  const uint32_t expected_version = 1;
  MEMCMP_EQUAL(coredump_buf, &expected_version, sizeof(expected_version));
  coredump_buf += sizeof(expected_version);
  const uint32_t segment_hdr_sz = 12;
  uint32_t total_length = 12 + sizeof(regs) + segment_hdr_sz;

  struct MemfaultDeviceInfo info;
  memfault_platform_get_device_info(&info);
  total_length += (5 * segment_hdr_sz) + strlen(info.device_serial) +
      strlen(info.hardware_version) + strlen(info.software_version) + strlen(info.software_type) + sizeof(uint32_t);

  const uint32_t pad_needed = (4 - (total_length % 4));
  total_length += pad_needed ? pad_needed + segment_hdr_sz : 0;

  total_length += sizeof(trace_reason) + segment_hdr_sz;

  for (size_t i = 0; i < s_num_fake_arch_regions; i++) {
    total_length += s_fake_arch_region[0].region_size + segment_hdr_sz;

    const uint32_t arch_padding = (4 - (total_length % 4));
    total_length += arch_padding ? arch_padding + segment_hdr_sz : 0;
  }

  for (size_t i = 0; i < s_num_fake_regions; i++) {
    total_length += s_fake_memory_region[i].region_size + segment_hdr_sz;
  }
  MEMCMP_EQUAL(&total_length, coredump_buf, sizeof(total_length));
  coredump_buf += sizeof(total_length);

  // registers are written first
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(&regs[0], coredump_buf, sizeof(regs));
  coredump_buf += sizeof(regs);

  // device info
  LONGS_EQUAL(2, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.device_serial, coredump_buf, strlen(info.device_serial));
  coredump_buf += strlen(info.device_serial);

  LONGS_EQUAL(10, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.software_version, coredump_buf, strlen(info.software_version));
  coredump_buf += strlen(info.software_version);

  LONGS_EQUAL(11, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.software_type, coredump_buf, strlen(info.software_type));
  coredump_buf += strlen(info.software_type);

  LONGS_EQUAL(4, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.hardware_version, coredump_buf, strlen(info.hardware_version));
  coredump_buf += strlen(info.hardware_version);

  LONGS_EQUAL(7, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  const uint32_t machine_type = 0;
  MEMCMP_EQUAL(&machine_type, coredump_buf, sizeof(machine_type));
  coredump_buf += sizeof(machine_type);

  // trace reason
  LONGS_EQUAL(5, *((uint32_t*)(uintptr_t)coredump_buf));
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(&trace_reason, coredump_buf, sizeof(trace_reason));
  coredump_buf += sizeof(trace_reason);

  // expected padding
  if (pad_needed) {
    LONGS_EQUAL(6, *((uint32_t*)(uintptr_t)coredump_buf));
    coredump_buf += segment_hdr_sz;
    coredump_buf += pad_needed;
  }

  // now we should find the architecture specific regions
  for (size_t i = 0; i < s_num_fake_arch_regions; i++) {
    // make sure regions are aligned on 4 byte boundaries
    LONGS_EQUAL(0, (uint64_t)coredump_buf % 4);

    size_t segment_size = s_fake_arch_region[i].region_size;
    coredump_buf += segment_hdr_sz;
    MEMCMP_EQUAL(s_fake_arch_region[i].region_start, coredump_buf, segment_size);
    coredump_buf += segment_size;

    const uint32_t arch_padding = (4 - ((uint64_t)coredump_buf % 4));
    coredump_buf += arch_padding + segment_hdr_sz;
  }

  for (size_t i = 0; i < s_num_fake_regions; i++) {
    // make sure regions are aligned on 4 byte boundaries
    LONGS_EQUAL(0, (uint64_t)coredump_buf % 4);

    size_t segment_size = s_fake_memory_region[i].region_size;
    coredump_buf += segment_hdr_sz;
    MEMCMP_EQUAL(s_fake_memory_region[i].region_start, coredump_buf, segment_size);
    coredump_buf += segment_size;
  }

  // we should see that a coredump is saved!
  size_t total_coredump_size = 0;
  bool has_coredump = memfault_coredump_has_valid_coredump(&total_coredump_size);
  CHECK(has_coredump);
  LONGS_EQUAL(total_length, total_coredump_size);
}

static bool prv_test_write_cb(const void *data, size_t data_len, void *ctx) {
  return mock().actualCall(__func__)
    .withMemoryBufferParameter("data", (const uint8_t *)data, data_len)
    .withPointerParameter("ctx", ctx)
    .returnBoolValueOrDefault(true);
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpWriteHeader) {
  const size_t test_size = 0x12345678;
  void *test_ctx = (void *)0x56789ABC;

  const uint8_t expected_data[] = {
      'C', 'O', 'R', 'E',
      0x01, 0x00, 0x00, 0x00,  // version 1
      0x78, 0x56, 0x34, 0x12,  // size
  };
  mock().expectOneCall("prv_test_write_cb")
    .withMemoryBufferParameter("data", expected_data, sizeof(expected_data))
    .withPointerParameter("ctx", test_ctx);

  CHECK_EQUAL(true, memfault_coredump_write_header(test_size, prv_test_write_cb, test_ctx));
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpWriteBlock) {
  void *test_ctx = (void *)0x56789ABC;
  const uint8_t test_payload[] = { 0x01, 0x02, 0x03, 0x04, };

  const uint8_t expected_block_header_data[] = {
      0x01,                    // type
      0x00, 0x00, 0x00,        // reserved
      0x00, 0x00, 0x00, 0x00,  // address
      0x04, 0x00, 0x00, 0x00,  // length
  };
  mock().expectOneCall("prv_test_write_cb")
        .withMemoryBufferParameter("data", expected_block_header_data, sizeof(expected_block_header_data))
        .withPointerParameter("ctx", test_ctx);
  mock().expectOneCall("prv_test_write_cb")
        .withMemoryBufferParameter("data", test_payload, sizeof(test_payload))
        .withPointerParameter("ctx", test_ctx);

  CHECK_EQUAL(true, memfault_coredump_write_block(kMfltCoredumpBlockType_MemoryRegion,
      test_payload, sizeof(test_payload), prv_test_write_cb, test_ctx));
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpWriteBlockNullPayload) {
  void *test_ctx = (void *)0x56789ABC;
  const size_t test_payload_size = 4;

  const uint8_t expected_block_header_data[] = {
      0x01,                    // type
      0x00, 0x00, 0x00,        // reserved
      0x00, 0x00, 0x00, 0x00,  // address
      0x04, 0x00, 0x00, 0x00,  // length
  };
  mock().expectOneCall("prv_test_write_cb")
        .withMemoryBufferParameter("data", expected_block_header_data, sizeof(expected_block_header_data))
        .withPointerParameter("ctx", test_ctx);

  CHECK_EQUAL(true, memfault_coredump_write_block(kMfltCoredumpBlockType_MemoryRegion,
                                                  NULL, test_payload_size, prv_test_write_cb, test_ctx));
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpWriteBlockWriteFailure) {
  void *test_ctx = (void *)0x56789ABC;
  const size_t test_payload_size = 4;

  mock().expectOneCall("prv_test_write_cb")
        .ignoreOtherParameters()
        .andReturnValue(false);

  CHECK_EQUAL(false, memfault_coredump_write_block(kMfltCoredumpBlockType_MemoryRegion,
                                                  NULL, test_payload_size, prv_test_write_cb, test_ctx));
}
