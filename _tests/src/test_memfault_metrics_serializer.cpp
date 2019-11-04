//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>
  #include <stdio.h>
  #include <stdint.h>

  #include "memfault/metrics/serializer.h"
  #include "memfault/metrics/storage.h"
  #include "memfault/metrics/utils.h"

  // a perfectly sized storage payload for our test pattern
  static uint8_t s_storage[175];
  static size_t s_storage_space_available;
  static uint32_t s_current_storage_offset = 0;
}

TEST_GROUP(MemfaultMetricsSerializer){
  void setup() {
    s_storage_space_available = sizeof(s_storage);
    s_current_storage_offset = 0;
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

//
// A very simple fake storage implementation
//

size_t memfault_metrics_heartbeat_storage_begin_write(void) {
  mock().actualCall(__func__);
  memset(s_storage, 0x0, sizeof(s_storage));
  return s_storage_space_available;
}

// offset not really needed by encoder
bool memfault_metrics_heartbeat_storage_append(const void *bytes, size_t num_bytes) {
  CHECK((s_current_storage_offset + num_bytes) <= s_storage_space_available);
  memcpy(&s_storage[s_current_storage_offset], bytes, num_bytes);
  s_current_storage_offset += num_bytes;
  return true;
}

void memfault_metrics_heartbeat_storage_finish_write(bool rollback) {
  mock().actualCall(__func__).withParameter("rollback", rollback);
}


//
// For the purposes of our serialization test, we will
// just serialize 1 of each supported type
//

size_t memfault_metrics_heartbeat_get_num_metrics(void) {
  return 2;
}

void memfault_metrics_heartbeat_iterate(MemfaultMetricIteratorCallback cb, void *ctx) {
  sMemfaultMetricInfo info = { 0 };
  info.key._impl = "unsigned_int";
  info.type = kMemfaultMetricType_Unsigned;
  info.val.u32 = 1000;
  cb(ctx, &info);

  info.key._impl = "signed_int";
  info.type = kMemfaultMetricType_Signed;
  info.val.i32 = -1000;
  cb(ctx, &info);
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricSerialize) {
  mock().expectOneCall("memfault_metrics_heartbeat_storage_begin_write");
  mock().expectOneCall("memfault_metrics_heartbeat_storage_finish_write").withParameter("rollback", false);

  memfault_metrics_heartbeat_serialize();

  const uint8_t expected_serialization[] = {
      0xA7, 0x64, 0x74, 0x79, 0x70, 0x65, 0x69, 0x68, 0x65, 0x61, 0x72, 0x74,
      0x62, 0x65, 0x61, 0x74, 0x6B, 0x73, 0x64, 0x6B, 0x5F, 0x76, 0x65, 0x72,
      0x73, 0x69, 0x6F, 0x6E, 0x65, 0x30, 0x2E, 0x35, 0x2E, 0x30, 0x6D, 0x64,
      0x65, 0x76, 0x69, 0x63, 0x65, 0x5F, 0x73, 0x65, 0x72, 0x69, 0x61, 0x6C,
      0x69, 0x44, 0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x6D, 0x73,
      0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x5F, 0x74, 0x79, 0x70, 0x65,
      0x64, 0x6D, 0x61, 0x69, 0x6E, 0x70, 0x73, 0x6F, 0x66, 0x74, 0x77, 0x61,
      0x72, 0x65, 0x5F, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x65, 0x31,
      0x2E, 0x32, 0x2E, 0x33, 0x70, 0x68, 0x61, 0x72, 0x64, 0x77, 0x61, 0x72,
      0x65, 0x5F, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x66, 0x65, 0x76,
      0x74, 0x5F, 0x32, 0x34, 0x6A, 0x65, 0x76, 0x65, 0x6E, 0x74, 0x5F, 0x69,
      0x6E, 0x66, 0x6F, 0xA1, 0x67, 0x6D, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73,
      0xA2, 0x6C, 0x75, 0x6E, 0x73, 0x69, 0x67, 0x6E, 0x65, 0x64, 0x5F, 0x69,
      0x6E, 0x74, 0x19, 0x03, 0xE8, 0x6A, 0x73, 0x69, 0x67, 0x6E, 0x65, 0x64,
      0x5F, 0x69, 0x6E, 0x74, 0x39, 0x03, 0xE7};

  LONGS_EQUAL(sizeof(expected_serialization), sizeof(s_storage));
  MEMCMP_EQUAL(expected_serialization, s_storage, sizeof(s_storage));
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricSerializeWorstCaseSize) {
  const size_t worst_case_storage = memfault_metrics_heartbeat_compute_worst_case_storage_size();
  LONGS_EQUAL(179, worst_case_storage);
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricSerializeOutOfSpace) {
  // iterate over all buffer sizes less than the encoding we need
  // this should exercise all early exit paths
  for (size_t i = 0; i < sizeof(s_storage) - 1; i++) {
    memset(s_storage, 0x0, sizeof(s_storage));
    s_storage_space_available = i;
    s_current_storage_offset = 0;

    mock().expectOneCall("memfault_metrics_heartbeat_storage_begin_write");
    mock().expectOneCall("memfault_metrics_heartbeat_storage_finish_write").withParameter("rollback", true);

    memfault_metrics_heartbeat_serialize();

    mock().checkExpectations();

    // make sure we didn't write past the end of the buffer
    uint8_t zero_buf[sizeof(s_storage) - s_storage_space_available];
    memset(zero_buf, 0x0, sizeof(zero_buf));
    MEMCMP_EQUAL(zero_buf, &s_storage[i], sizeof(zero_buf));
  }

}
