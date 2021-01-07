//! @file
//!

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"

#include <stddef.h>
#include <stdint.h>

#include "memfault/panics/platform/coredump.h"
#include "memfault/core/math.h"
#include "fakes/fake_memfault_buffered_coredump_storage.h"

TEST_GROUP(MemfaultBufferedCoredumpStorage){
  void setup() {
    fake_buffered_coredump_storage_reset();
  }
  void teardown() { }
};

static void prv_validate_pattern_written(size_t bytes_to_check) {
  for (size_t i = 0; i < bytes_to_check; i++) {
    LONGS_EQUAL(i, g_ram_backed_storage[i]);
  }
}

TEST(MemfaultBufferedCoredumpStorage, Test_BufferedStorageSingleByte) {
  const size_t size_to_write = (MEMFAULT_STORAGE_SIZE / 2) + 1;

  uint32_t start_offset = 2;
  for (size_t i = 0; i < size_to_write; i++) {
    const uint32_t addr = (i + start_offset) % size_to_write;
    uint8_t data = addr & 0xff;
    const bool success = memfault_platform_coredump_storage_write(addr, &data, sizeof(data));
    CHECK(success);
  }

  prv_validate_pattern_written(size_to_write);
}

TEST(MemfaultBufferedCoredumpStorage, Test_BufferedStorageWord) {
  uint32_t start_offset = 3;
  uint8_t data[4];

  for (size_t i = 0; i < MEMFAULT_STORAGE_SIZE ; i += sizeof(data)) {
    const uint32_t addr = (i + start_offset) % MEMFAULT_STORAGE_SIZE;
    const size_t write_len = MEMFAULT_MIN(MEMFAULT_STORAGE_SIZE - addr,
                                        sizeof(data));
    if (write_len != sizeof(data)) {
      // we are wrapping around so do single byte writes
      for (size_t j = 0; j < sizeof(data); j++) {
        const uint32_t byte_addr = (addr + j) % MEMFAULT_STORAGE_SIZE;
        data[0] = byte_addr & 0xff;
        const bool success = memfault_platform_coredump_storage_write(byte_addr, &data, 1);
        CHECK(success);
      }
    } else {
      // write a word
      for (size_t j = 0; j < sizeof(data); j++) {
        data[j] = (addr + j) & 0xff;
      }

      const bool success = memfault_platform_coredump_storage_write(addr, &data[0], sizeof(data));
      CHECK(success);
    }
  }

  prv_validate_pattern_written(MEMFAULT_STORAGE_SIZE);
}

TEST(MemfaultBufferedCoredumpStorage, Test_LargeWrite) {
  uint32_t start_offset = 1;
  uint8_t data[MEMFAULT_STORAGE_SIZE - start_offset];
  for (size_t i = 0; i < sizeof(data); i++) {
    data[i] = (start_offset + i) & 0xff;
  }

  bool success = memfault_platform_coredump_storage_write(start_offset, &data, sizeof(data));
  CHECK(success);

  // write final byte to complete update
  data[0] = 0;
  success = memfault_platform_coredump_storage_write(0, &data, 1);
  CHECK(success);

  prv_validate_pattern_written(MEMFAULT_STORAGE_SIZE);
}


TEST(MemfaultBufferedCoredumpStorage, Test_BadSectorSize) {
  fake_buffered_coredump_storage_set_size(7);
  char data[32] = { 0 };
  const bool success = memfault_platform_coredump_storage_write(0, data, sizeof(data));
  CHECK(!success);
}

TEST(MemfaultBufferedCoredumpStorage, Test_BadBlockWrite) {

  char data[32] = { 0 };

  fake_buffered_coredump_inject_write_failure();
  bool success = memfault_platform_coredump_storage_write(32, data, sizeof(data));
  CHECK(!success);


  fake_buffered_coredump_inject_write_failure();
  success = memfault_platform_coredump_storage_write(0, data, sizeof(data));
  CHECK(!success);
}
