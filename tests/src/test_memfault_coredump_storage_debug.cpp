//! @file

#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"

static bool s_inject_prepare_failure = false;
bool memfault_platform_coredump_save_begin(void) {
  if (s_inject_prepare_failure) {
    return false;
  }

  return true;
}

static uint8_t s_ram_backed_coredump_region[200];
#define COREDUMP_REGION_SIZE sizeof(s_ram_backed_coredump_region)

static bool s_inject_get_info_failure = false;
void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info) {
  if (s_inject_get_info_failure) {
    return; // fail to populate information
  }

  const size_t coredump_region_size = sizeof(s_ram_backed_coredump_region);

  *info = (sMfltCoredumpStorageInfo) {
    .size = coredump_region_size,
    .sector_size = coredump_region_size
  };
}

static int s_inject_read_failure_offset;

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((offset + read_len) > COREDUMP_REGION_SIZE) {
    return false;
  }


  const uint8_t *read_ptr = &s_ram_backed_coredump_region[offset];

  if ((s_inject_read_failure_offset >= 0) &&
      (s_inject_read_failure_offset < (int)COREDUMP_REGION_SIZE)) {
    s_ram_backed_coredump_region[s_inject_read_failure_offset] = 0xff;
  }

  memcpy(data, read_ptr, read_len);

  // let's have most of the failures be fake successes and for one case
  // just return a driver read failure
  return (s_inject_read_failure_offset != 0);
}

bool memfault_coredump_read(uint32_t offset, void *data,
                            size_t read_len) {
  return memfault_platform_coredump_storage_read(offset, data, read_len);
}

typedef enum {
  kMemfaultEraseFailureMode_None = 0,
  kMemfaultEraseFailureMode_ClearFailure,
  kMemfaultEraseFailureMode_EraseDriverFailure,

  kMemfaultEraseFailureMode_NumModes,
} eMemfaultEraseFailureMode;

static eMemfaultEraseFailureMode s_inject_erase_failure;

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > COREDUMP_REGION_SIZE) {
    return false;
  }

  uint8_t *erase_ptr = &s_ram_backed_coredump_region[offset];
  memset(erase_ptr, 0x0, erase_size);

  switch ((int)s_inject_erase_failure) {
    case kMemfaultEraseFailureMode_ClearFailure:
      s_ram_backed_coredump_region[COREDUMP_REGION_SIZE / 2] = 0xa5;
      break;
    case kMemfaultEraseFailureMode_EraseDriverFailure:
      return false;
    default:
      break;
  }
  return true;
}

typedef enum {
  kMemfaultWriteFailureMode_None = 0,
  kMemfaultWriteFailureMode_WriteDriverFailure,
  kMemfaultWriteFailureMode_WordUnaligned,
  kMemfaultWriteFailureMode_PartialWriteFail,
  kMemfaultWriteFailureMode_LastByte,
  kMemfaultWriteFailureMode_WriteFailureAtOffset0,
  kMemfaultWriteFailureMode_ReadAfterWrite,

  kMemfaultWriteFailureMode_NumModes,
} eMemfaultWriteFailureMode;

static eMemfaultWriteFailureMode s_inject_write_failure;

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > COREDUMP_REGION_SIZE) {
    return false;
  }

  switch ((int)s_inject_write_failure) {
    case kMemfaultWriteFailureMode_WriteDriverFailure:
      return false;

    case kMemfaultWriteFailureMode_WriteFailureAtOffset0:
      if (offset == 0) {
        return false;
      }
      break;

    case kMemfaultWriteFailureMode_WordUnaligned:
      if ((offset % 4) != 0) {
        return true; // silent fail
      }
      break;
    case kMemfaultWriteFailureMode_PartialWriteFail:
      // fail to write 1 byte
      data_len -= 1;
      break;
    case kMemfaultWriteFailureMode_LastByte:
      if ((offset + data_len) == COREDUMP_REGION_SIZE) {
        data_len -= 1;
      }
      break;
    case kMemfaultWriteFailureMode_ReadAfterWrite:
      s_inject_read_failure_offset = 0;
      break;
    default:
      break;
  }

  uint8_t *write_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

void memfault_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  memfault_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}

TEST_GROUP(MfltCoredumpStorageTestGroup) {
  void setup() {
    s_inject_write_failure = kMemfaultWriteFailureMode_None;
    s_inject_read_failure_offset = -1;
    s_inject_erase_failure = kMemfaultEraseFailureMode_None;
    s_inject_get_info_failure = false;
    s_inject_prepare_failure = false;
  }

  void teardown() {
  }
};

TEST(MfltCoredumpStorageTestGroup, Test_StorageImplementationGood) {
  bool success = memfault_coredump_storage_debug_test_begin();
  success &= memfault_coredump_storage_debug_test_finish();
  CHECK(success);
}

TEST(MfltCoredumpStorageTestGroup, Test_WriteFailureModes) {
  for (int i = 1; i < kMemfaultWriteFailureMode_NumModes; i++) {
    s_inject_write_failure = (eMemfaultWriteFailureMode)i;
    s_inject_read_failure_offset = -1;
    bool success = memfault_coredump_storage_debug_test_begin();
    success &= memfault_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(MfltCoredumpStorageTestGroup, Test_ReadCompareFailure) {
  for (size_t i = 0; i < COREDUMP_REGION_SIZE; i++) {
    s_inject_read_failure_offset = i;
    bool success = memfault_coredump_storage_debug_test_begin();
    success &= memfault_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(MfltCoredumpStorageTestGroup, Test_EraseFailure) {
  for (int i = 1; i < kMemfaultEraseFailureMode_NumModes; i++) {
    s_inject_erase_failure = (eMemfaultEraseFailureMode)i;
    bool success = memfault_coredump_storage_debug_test_begin();
    success &= memfault_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(MfltCoredumpStorageTestGroup, Test_ClearFailureDueToWriteFailure) {
  bool success = memfault_coredump_storage_debug_test_begin();
  CHECK(success);

  s_inject_write_failure = (eMemfaultWriteFailureMode)kMemfaultWriteFailureMode_PartialWriteFail;
  success = memfault_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(MfltCoredumpStorageTestGroup, Test_ClearFailureDueToReadFailure) {
  bool success = memfault_coredump_storage_debug_test_begin();
  CHECK(success);

  s_inject_write_failure = (eMemfaultWriteFailureMode)kMemfaultWriteFailureMode_ReadAfterWrite;
  success = memfault_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(MfltCoredumpStorageTestGroup, Test_GetInfoFail) {
  s_inject_get_info_failure = true;
  bool success = memfault_coredump_storage_debug_test_begin();
  success &= memfault_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(MfltCoredumpStorageTestGroup, Test_PrepareFail) {
  s_inject_prepare_failure = true;
  bool success = memfault_coredump_storage_debug_test_begin();
  success &= memfault_coredump_storage_debug_test_finish();
  CHECK(!success);
}
