#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

extern "C" {
  #include "fakes/fake_memfault_platform_coredump_storage.h"
  #include "memfault/core/build_info.h"
  #include "memfault/core/compiler.h"
  #include "memfault/core/platform/core.h"
  #include "memfault/core/platform/device_info.h"
  #include "memfault/panics/coredump.h"
  #include "memfault/panics/coredump_impl.h"
  #include "memfault/panics/platform/coredump.h"

  MEMFAULT_ALIGNED(0x8) static uint8_t s_storage_buf[4 * 1024];

  static sMfltCoredumpRegion s_fake_memory_region[2];
  static size_t s_num_fake_regions;

  static sMfltCoredumpRegion s_fake_arch_region[3];
  static size_t s_num_fake_arch_regions;

  static sMfltCoredumpRegion s_fake_sdk_region[2];
  static size_t s_num_fake_sdk_regions;

  static const uint8_t s_fake_memfault_build_id[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0f, 0x10, 0x11, 0x12, 0x13
  };

  void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
    *info = (struct MemfaultDeviceInfo) {
      .device_serial = "1",
      .software_type = "main",
      .software_version = "22",
      .hardware_version = "333",
    };
  }

  bool memfault_coredump_read(uint32_t offset, void *buf, size_t buf_len) {
    mock().actualCall(__func__);
    return fake_memfault_platform_coredump_storage_read(offset, buf, buf_len);
  }

  bool memfault_platform_coredump_storage_read(uint32_t offset, void *buf,
                                               size_t buf_len) {
    mock().actualCall(__func__);
    return fake_memfault_platform_coredump_storage_read(offset, buf, buf_len);
  }

  bool memfault_build_info_read(sMemfaultBuildInfo *info) {
    const bool build_info_available = mock().actualCall(__func__).returnBoolValueOrDefault(false);
    if (build_info_available) {
      memcpy(info->build_id, s_fake_memfault_build_id, sizeof(s_fake_memfault_build_id));
    }
    return build_info_available;
  }
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    MEMFAULT_UNUSED const sCoredumpCrashInfo *crash_info,
    size_t *num_regions) {
  *num_regions = s_num_fake_regions;
  return &s_fake_memory_region[0];
}

const sMfltCoredumpRegion *memfault_coredump_get_arch_regions(size_t *num_regions) {
  *num_regions = s_num_fake_arch_regions;
  return &s_fake_arch_region[0];
}

const sMfltCoredumpRegion *memfault_coredump_get_sdk_regions(size_t *num_regions) {
  *num_regions = s_num_fake_sdk_regions;
  return &s_fake_sdk_region[0];
}

TEST_GROUP(MfltCoredumpTestGroup) {
  void setup() {
    mock().enable();
    fake_memfault_platform_coredump_storage_setup(s_storage_buf, sizeof(s_storage_buf), 1024);
    memfault_platform_coredump_storage_erase(0, sizeof(s_storage_buf));

    static uint8_t s_fake_core_region0[] = { 'h', 'e', 'l', 'l', 'o','!','!','!' };
    static uint32_t s_fake_core_region1 = 0x61626364;
    s_fake_memory_region[0].type = kMfltCoredumpRegionType_Memory;
    s_fake_memory_region[0].region_start = &s_fake_core_region0;
    s_fake_memory_region[0].region_size = sizeof(s_fake_core_region0);

    s_fake_memory_region[1].type = kMfltCoredumpRegionType_MemoryWordAccessOnly;
    s_fake_memory_region[1].region_start = &s_fake_core_region1;
    s_fake_memory_region[1].region_size = sizeof(s_fake_core_region1);

    s_num_fake_regions = 2;

    static uint8_t s_fake_arch_region1[] = { 'e', 'f', 'g' };
    s_fake_arch_region[0].type = kMfltCoredumpRegionType_Memory;
    s_fake_arch_region[0].region_start = &s_fake_arch_region1;
    s_fake_arch_region[0].region_size = sizeof(s_fake_arch_region1);

    // Valid cache of "registers" starting at 0xE0000000.
    static uint32_t s_fake_arch_cached_block_region[(sizeof(sMfltCachedBlock) + 3*4)/4];
    sMfltCachedBlock *blk = (sMfltCachedBlock *)&s_fake_arch_cached_block_region[0];
    blk->blk_size = sizeof(s_fake_arch_cached_block_region) - sizeof(sMfltCachedBlock);
    blk->cached_address = 0xE0000000;
    blk->blk[0] = 0x12345678;
    blk->blk[1] = 0xAABBCCDD;
    blk->blk[2] = 0x9999EEEE;
    blk->valid_cache = 1;

    s_fake_arch_region[1].type = kMfltCoredumpRegionType_CachedMemory;
    s_fake_arch_region[1].region_start = &s_fake_arch_cached_block_region;
    s_fake_arch_region[1].region_size = sizeof(s_fake_arch_cached_block_region);

    // Invalid cache of "registers" starting at 0xE000E000.
    static uint32_t s_fake_arch_cached_block_region_invalid[(sizeof(sMfltCachedBlock) + 3*4)/4];
    blk = (sMfltCachedBlock *)&s_fake_arch_cached_block_region_invalid[0];
    blk->blk_size = sizeof(s_fake_arch_cached_block_region_invalid) - sizeof(sMfltCachedBlock);
    blk->cached_address = 0xE000E000;
    blk->blk[0] = 0x77777777;
    blk->blk[1] = 0x88888888;
    blk->blk[2] = 0xFEEDFACE;
    blk->valid_cache = 0;

    s_fake_arch_region[2].type = kMfltCoredumpRegionType_CachedMemory;
    s_fake_arch_region[2].region_start = &s_fake_arch_cached_block_region_invalid;
    s_fake_arch_region[2].region_size = sizeof(s_fake_arch_cached_block_region_invalid);

    s_num_fake_arch_regions = 3;

    static uint8_t s_fake_sdk_region1[] = { 'a', 'b', 'c' };
    s_fake_sdk_region[0].type = kMfltCoredumpRegionType_Memory;
    s_fake_sdk_region[0].region_start = &s_fake_sdk_region1;
    s_fake_sdk_region[0].region_size = sizeof(s_fake_sdk_region1);

    static uint8_t s_fake_sdk_region2[] = { 'd', 'e', 'f' };
    s_fake_sdk_region[1].type = kMfltCoredumpRegionType_Memory;
    s_fake_sdk_region[1].region_start = &s_fake_sdk_region2;
    s_fake_sdk_region[1].region_size = sizeof(s_fake_sdk_region2);

    s_num_fake_sdk_regions = 2;
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static bool prv_check_coredump_validity_and_get_size(size_t *total_coredump_size) {
  // NB: This should only be called while the system is running so memfault_coredump_read
  // should always be used. Let's use a mock to verify that.
  mock().expectOneCall("memfault_coredump_read");
  const bool has_coredump = memfault_coredump_has_valid_coredump(total_coredump_size);
  mock().checkExpectations();
  return has_coredump;
}

static void prv_assert_storage_empty(void) {
  size_t total_coredump_size;

  bool has_coredump = prv_check_coredump_validity_and_get_size(&total_coredump_size);
  CHECK(!has_coredump);

  uint8_t empty_storage[sizeof(s_storage_buf)];
  memset(empty_storage, 0xff, sizeof(empty_storage));
  MEMCMP_EQUAL(empty_storage, s_storage_buf, sizeof(s_storage_buf));
}

static bool prv_collect_regions_and_save(void *regs, size_t size,
                                         uint32_t trace_reason) {
  size_t num_regions = 0;
  const sMfltCoredumpRegion *regions =
      memfault_platform_coredump_get_regions(NULL, &num_regions);

  if (num_regions != 0) {
    // we are going to try and write a coredump so this should trigger a header read
    mock().expectOneCall("memfault_platform_coredump_storage_read");
    mock().ignoreOtherCalls(); // We will test handling memfault_build_info_read() explicitly
  }

  sMemfaultCoredumpSaveInfo info = {
    .regs = regs,
    .regs_size = size,
    .trace_reason = (eMemfaultRebootReason)trace_reason,
    .regions = regions,
    .num_regions = num_regions,
  };
  const bool success = memfault_coredump_save(&info);
  mock().checkExpectations();
  return success;
}

static size_t prv_compute_space_needed_with_build_id(void *regs, size_t size, uint32_t trace_reason,
                                                     bool include_build_id) {
  mock().checkExpectations();
  // computing the space needed to save a coredump shouldn't require any access to the storage
  // medium itself
  mock().expectNCalls(0, "memfault_coredump_read");
  mock().expectNCalls(0, "memfault_platform_coredump_storage_read");

  // if there are no regions, no coredump will be saved so the build id call will never be invoked
  if (s_num_fake_regions != 0) {
    mock().expectOneCall("memfault_build_info_read").andReturnValue(include_build_id);
  }
  size_t num_regions = 0;
  const sMfltCoredumpRegion *regions =
      memfault_platform_coredump_get_regions(NULL, &num_regions);
  sMemfaultCoredumpSaveInfo info = {
    .regs = regs,
    .regs_size = size,
    .trace_reason = (eMemfaultRebootReason)trace_reason,
    .regions = regions,
    .num_regions = num_regions,
  };
  const size_t size_needed = memfault_coredump_get_save_size(&info);
  mock().checkExpectations();
  mock().clear();
  return size_needed;
}

static size_t prv_compute_space_needed(void *regs, size_t size,
                                       uint32_t trace_reason) {
  const bool include_build_id = false;
  return prv_compute_space_needed_with_build_id(regs, size, trace_reason, include_build_id);
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpNoRegions) {
  s_num_fake_regions = 0;
  s_num_fake_arch_regions = 0;
  s_num_fake_sdk_regions = 0;

  prv_collect_regions_and_save(NULL, 0, 0);
  size_t size = prv_compute_space_needed(NULL, 0, 0);
  LONGS_EQUAL(0, size);

  prv_assert_storage_empty();
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpEmptyStorage) {
  fake_memfault_platform_coredump_storage_setup(NULL, 0, 1024);

  size_t total_size;
  const bool has_coredump = memfault_coredump_has_valid_coredump(&total_size);
  CHECK(!has_coredump);
}


TEST(MfltCoredumpTestGroup, Test_MfltCoredumpNothinWritten) {
  prv_assert_storage_empty();
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpStorageTooSmall) {
  const uint32_t regs[] = { 0x10111213, 0x20212223, 0x30313233, 0x40414243, 0x50515253 };
  const uint32_t trace_reason = 0xdeadbeef;

  // update coredump_size_without_build_id if you add/remove regions.
  const size_t coredump_size_without_build_id = 308;
  const size_t coredump_size_with_build_id = coredump_size_without_build_id + 20 /* sha1 */ + 12 /* sMfltCoredumpBlock */;
  for (size_t i = 1; i <= coredump_size_with_build_id; i++) {
    uint8_t storage[i];
    memset(storage, 0x0, sizeof(storage));
    fake_memfault_platform_coredump_storage_setup(storage, i, i);
    memfault_platform_coredump_storage_clear();

    // build id is written after the header and regs
    const uint32_t segment_hdr_sz = 12;

    const size_t build_id_start_offset = (12 + sizeof(regs) + segment_hdr_sz + 16 /* reserved for footer */);
    const bool do_collect_build_id = (i >= (build_id_start_offset + 3));

    if (i >= build_id_start_offset) {
      mock().expectOneCall("memfault_build_info_read").andReturnValue(do_collect_build_id);
    }
    bool success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);

    // even if storage itself is too small we should always be able to compute the amount of space
    // needed!
    const size_t space_needed = prv_compute_space_needed_with_build_id((void *)&regs, sizeof(regs),
                                                                       trace_reason, do_collect_build_id);

    const size_t expected_coredump_size = do_collect_build_id ? coredump_size_with_build_id :
                                                                coredump_size_without_build_id;
    LONGS_EQUAL(expected_coredump_size, space_needed);

    // once we get to the memory regions section, the save can succeed, we just won't get all the memory regions!
    CHECK(success == (i >= 182));
  }
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpTruncated) {
  const uint32_t regs[] = { 0xabababab };
  const uint32_t trace_reason = 0xdeadbeef;

  // get the total size of the coredump, update space_needed
  // check if you add/remove regions.
  const bool do_collect_build_id = false;
  fake_memfault_platform_coredump_storage_setup(NULL, 0, 0);
  const size_t space_needed = prv_compute_space_needed_with_build_id(
      (void *)&regs, sizeof(regs), trace_reason, do_collect_build_id);
  LONGS_EQUAL(292, space_needed);

  size_t num_regions;
  const sMfltCoredumpRegion *regions =
      memfault_platform_coredump_get_regions(NULL, &num_regions);
  // this should match the number of fake regions we have in s_num_fake_regions
  LONGS_EQUAL(2, num_regions);


  const sMfltCoredumpRegion *last_region = &regions[num_regions - 1];
  const size_t block_hdr_len = 12;

  uint8_t storage[space_needed];
  memset(&storage, 0x0, sizeof(storage));

  // size such that the second to last region gets truncated
  size_t storage_size = space_needed - last_region->region_size - block_hdr_len - 1;

  fake_memfault_platform_coredump_storage_setup(storage, storage_size, storage_size);
  bool success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(success);

  size_t coredump_size;
  bool has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(has_coredump);
  // second to last region should be truncated by one word
  LONGS_EQUAL(storage_size - 3 , coredump_size);
  memfault_platform_coredump_storage_clear();

  // size with not enough space for the last region
  storage_size = space_needed - last_region->region_size - block_hdr_len;
  for (size_t i = 0; i < block_hdr_len + 3; i++) {
    fake_memfault_platform_coredump_storage_setup(storage, storage_size + i, storage_size + i);
    success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
    CHECK(success);

    has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
    CHECK(has_coredump);
    LONGS_EQUAL(storage_size, coredump_size);
    memfault_platform_coredump_storage_clear();
  }

  // space for beginning of last region so space used should increase
  storage_size += block_hdr_len + 4;

  fake_memfault_platform_coredump_storage_setup(storage, storage_size, storage_size);
  success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(success);

  has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(has_coredump);
  LONGS_EQUAL(storage_size, coredump_size);

  memfault_platform_coredump_storage_clear();
}

TEST(MfltCoredumpTestGroup, Test_MfltCoredumpNoOverwrite) {
  const uint32_t regs[] = { 0x10111213, 0x20212223, 0x30313233, 0x40414243, 0x50515253 };
  const uint32_t trace_reason = 0xdeadbeef;

  bool success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(success);

  // since we already have an unread core, it shouldn't be over
  success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(!success);
}

TEST(MfltCoredumpTestGroup, Test_BadMagic) {
  const uint32_t regs[] = { 0x10111213, 0x20212223, 0x30313233, 0x40414243, 0x50515253 };
  const uint32_t trace_reason = 0xdeadbeef;

  const bool success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(success);

  size_t coredump_size;
  bool has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(has_coredump);

  // now corrupt the magic
  memset(&s_storage_buf[1], 0xAB, 8);
  has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(!has_coredump);
}

TEST(MfltCoredumpTestGroup, Test_InvalidHeader) {
  const uint32_t regs[] = { 0x10111213, 0x20212223, 0x30313233, 0x40414243, 0x50515253 };
  const uint32_t trace_reason = 0xdeadbeef;

  const bool success = prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);
  CHECK(success);

  size_t coredump_size;
  bool has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(has_coredump);

  memfault_platform_coredump_storage_clear();
  has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(!has_coredump);
}

TEST(MfltCoredumpTestGroup, Test_ShortHeaderRead) {
  uint8_t storage;
  fake_memfault_platform_coredump_storage_setup(&storage, sizeof(storage), sizeof(storage));

  // A misconfiguration where coredump storage isn't even large enough to hold a header
  size_t coredump_size;
  const bool has_coredump = prv_check_coredump_validity_and_get_size(&coredump_size);
  CHECK(!has_coredump);
}

TEST(MfltCoredumpTestGroup, Test_CoredumpReadHeaderMagic) {
  const uint32_t regs[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };
  const uint32_t trace_reason = 0xdead;

  prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);

  uint32_t word = 0;
  const bool success = fake_memfault_platform_coredump_storage_read(0, &word, sizeof(word));
  CHECK(success);
  LONGS_EQUAL(0x45524f43, word);
}

// Returns correct region size for any region even if cached. Invalid
// cached blocks result in zero length so they are properly ignored.
static size_t prv_get_region_size(const sMfltCoredumpRegion *const region) {
  if (region->type != kMfltCoredumpRegionType_CachedMemory) {
    return region->region_size;
  }

  const sMfltCachedBlock *cached_block = (const sMfltCachedBlock *)region->region_start;
  return cached_block->valid_cache ? cached_block->blk_size : 0;
}

// Returns correct region start for any region even if cached.
static const void *prv_get_region_start(const sMfltCoredumpRegion *const region) {
  if (region->type != kMfltCoredumpRegionType_CachedMemory) {
    return region->region_start;
  }

  const sMfltCachedBlock *cached_block = (const sMfltCachedBlock *)region->region_start;
  return cached_block->blk;
}

static size_t prv_compute_padding_needed(uint64_t const offset) {
  return (offset % 4) ? 4 - (offset % 4) : 0;
}

// Test the basics ... make sure the coredump is flushed out in the order we expect
TEST(MfltCoredumpTestGroup, Test_MfltCoredumpSaveCore) {
  const uint32_t regs[] = { 0x1, 0x2, 0x3, 0x4, 0x5 };
  const uint32_t trace_reason = 0xdead;

  mock().expectOneCall("memfault_build_info_read").andReturnValue(true);

  prv_collect_regions_and_save((void *)&regs, sizeof(regs), trace_reason);

  uint8_t *coredump_buf = &s_storage_buf[0];
  const uint32_t expected_header_magic = 0x45524f43;
  MEMCMP_EQUAL(coredump_buf, &expected_header_magic, sizeof(expected_header_magic));
  coredump_buf += sizeof(expected_header_magic);
  const uint32_t expected_version = 2;
  MEMCMP_EQUAL(coredump_buf, &expected_version, sizeof(expected_version));
  coredump_buf += sizeof(expected_version);
  const uint32_t segment_hdr_sz = 12;
  uint32_t total_length = 12 + sizeof(regs) + segment_hdr_sz;

  struct MemfaultDeviceInfo info;
  memfault_platform_get_device_info(&info);
  total_length += (6 * segment_hdr_sz) + sizeof(s_fake_memfault_build_id) + strlen(info.device_serial) +
      strlen(info.hardware_version) + strlen(info.software_version) + strlen(info.software_type) + sizeof(uint32_t);

  const uint32_t pad_needed = prv_compute_padding_needed(total_length);
  total_length += pad_needed ? pad_needed + segment_hdr_sz : 0;

  total_length += sizeof(trace_reason) + segment_hdr_sz;

  for (size_t i = 0; i < s_num_fake_arch_regions; i++) {
    const size_t arch_region_size = prv_get_region_size(&s_fake_arch_region[i]);
    if (arch_region_size == 0) {
      // Skip invalid cached regions.
      continue;
    }

    total_length += arch_region_size + segment_hdr_sz;
    const uint32_t arch_padding = prv_compute_padding_needed(total_length);
    total_length += arch_padding ? arch_padding + segment_hdr_sz : 0;
  }

  for (size_t i = 0; i < s_num_fake_sdk_regions; i++) {
    const size_t sdk_region_size = prv_get_region_size(&s_fake_sdk_region[i]);
    if (sdk_region_size == 0) {
      continue;
    }

    total_length += sdk_region_size + segment_hdr_sz;
    const uint32_t sdk_padding = prv_compute_padding_needed(total_length);
    total_length += sdk_padding ? sdk_padding + segment_hdr_sz : 0;
  }

  for (size_t i = 0; i < s_num_fake_regions; i++) {
    const size_t mem_region_size = prv_get_region_size(&s_fake_memory_region[i]);
    if (mem_region_size == 0) {
      continue;
    }

    total_length += mem_region_size + segment_hdr_sz;
  }

  total_length += 16; // footer

  MEMCMP_EQUAL(&total_length, coredump_buf, sizeof(total_length));
  coredump_buf += sizeof(total_length);

  // registers are written first
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(&regs[0], coredump_buf, sizeof(regs));
  coredump_buf += sizeof(regs);

  // device info
  uint32_t actual_value;
  // memcpy for possibly unaligned address
  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(12, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(s_fake_memfault_build_id, coredump_buf, sizeof(s_fake_memfault_build_id));
  coredump_buf += sizeof(s_fake_memfault_build_id);

  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(2, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.device_serial, coredump_buf, strlen(info.device_serial));
  coredump_buf += strlen(info.device_serial);

  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(10, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.software_version, coredump_buf, strlen(info.software_version));
  coredump_buf += strlen(info.software_version);

  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(11, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.software_type, coredump_buf, strlen(info.software_type));
  coredump_buf += strlen(info.software_type);

  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(4, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(info.hardware_version, coredump_buf, strlen(info.hardware_version));
  coredump_buf += strlen(info.hardware_version);

  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(7, actual_value);
  coredump_buf += segment_hdr_sz;
  const uint32_t machine_type = 0;
  MEMCMP_EQUAL(&machine_type, coredump_buf, sizeof(machine_type));
  coredump_buf += sizeof(machine_type);

  // trace reason
  memcpy(&actual_value, coredump_buf, sizeof(actual_value));
  LONGS_EQUAL(5, actual_value);
  coredump_buf += segment_hdr_sz;
  MEMCMP_EQUAL(&trace_reason, coredump_buf, sizeof(trace_reason));
  coredump_buf += sizeof(trace_reason);

  // expected padding
  if (pad_needed) {
    memcpy(&actual_value, coredump_buf, sizeof(actual_value));
    LONGS_EQUAL(6, actual_value);
    coredump_buf += segment_hdr_sz;
    coredump_buf += pad_needed;
  }

  // now we should find the architecture specific regions
  for (size_t i = 0; i < s_num_fake_arch_regions; i++) {
    // make sure regions are aligned on 4 byte boundaries
    LONGS_EQUAL(0, (uint64_t)coredump_buf % 4);

    size_t segment_size = prv_get_region_size(&s_fake_arch_region[i]);
    if (segment_size == 0) {
      // Invalid cached block, skip it.
      continue;
    }
    coredump_buf += segment_hdr_sz;

    const void *memory = prv_get_region_start(&s_fake_arch_region[i]);
    MEMCMP_EQUAL(memory, coredump_buf, segment_size);
    coredump_buf += segment_size;

    if (((uint64_t)coredump_buf % 4) != 0) {
      const uint32_t arch_padding = (4 - ((uint64_t)coredump_buf % 4));
      coredump_buf += arch_padding + segment_hdr_sz;
    }
  }

  for (size_t i = 0; i < s_num_fake_sdk_regions; i++) {
    // make sure regions are aligned on 4 byte boundaries
    LONGS_EQUAL(0, (uint64_t)coredump_buf % 4);

    size_t segment_size = prv_get_region_size(&s_fake_sdk_region[i]);
    if (segment_size == 0) {
      continue;
    }
    coredump_buf += segment_hdr_sz;

    const void *memory = prv_get_region_start(&s_fake_sdk_region[i]);
    MEMCMP_EQUAL(memory, coredump_buf, segment_size);
    coredump_buf += segment_size;

    const uint32_t sdk_padding = prv_compute_padding_needed((uint64_t)coredump_buf);
    coredump_buf += sdk_padding + segment_hdr_sz;
  }

  for (size_t i = 0; i < s_num_fake_regions; i++) {
    // make sure regions are aligned on 4 byte boundaries
    LONGS_EQUAL(0, (uint64_t)coredump_buf % 4);

    size_t segment_size = prv_get_region_size(&s_fake_memory_region[i]);
    if (segment_size == 0) {
      continue;
    }
    coredump_buf += segment_hdr_sz;

    const void *memory = prv_get_region_start(&s_fake_memory_region[i]);
    MEMCMP_EQUAL(memory, coredump_buf, segment_size);
    coredump_buf += segment_size;
  }

  // check the footer
  const uint32_t expected_footer_magic = 0x504d5544;
  MEMCMP_EQUAL(coredump_buf, &expected_footer_magic, sizeof(expected_footer_magic));
  coredump_buf += sizeof(expected_footer_magic);
  uint32_t rsvd[] = {0x0, 0x0, 0x0};
  MEMCMP_EQUAL(coredump_buf, &rsvd[0], sizeof(rsvd));

  // should have been no calls to memfault_coredump_read
  mock().checkExpectations();

  // we should see that a coredump is saved!
  size_t total_coredump_size = 0;
  bool has_coredump = prv_check_coredump_validity_and_get_size(&total_coredump_size);
  CHECK(has_coredump);
  LONGS_EQUAL(total_length, total_coredump_size);
}
