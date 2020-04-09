//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "memfault/core/serializer_helper.h"
#include "memfault/util/cbor.h"

TEST_GROUP(MemfaultMetricsSerializerHelper){
  void setup() {
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_write_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  CHECK(ctx != NULL);
  uint8_t *result_buf = (uint8_t *)ctx;
  memcpy(&result_buf[offset], buf, buf_len);
}

static void prv_encode_metadata_and_check(sMemfaultCborEncoder *e, void *result, const size_t expected_size) {
  // walk through encodes up to the expected size to make sure they all fail out appropriately
  for (size_t i = 1; i <= expected_size; i++) {
    memset(result, 0x0, expected_size);
    memfault_cbor_encoder_init(e, prv_write_cb, result, i);
    const bool success = memfault_serializer_helper_encode_metadata(e, kMemfaultEventType_Heartbeat);
    const bool success_expected = (i == expected_size);
    LONGS_EQUAL(success_expected, success);
  }

  const size_t bytes_encoded = memfault_cbor_encoder_deinit(e);
  LONGS_EQUAL(expected_size, bytes_encoded);
}

TEST(MemfaultMetricsSerializerHelper, Test_MemfaultMetricSerializeEventMetadata) {
  uint8_t result[37];
  sMemfaultCborEncoder encoder;
  prv_encode_metadata_and_check(&encoder, result, sizeof(result));


  const uint8_t expected_result[] = {
    0xa7,
    // Type
    0x02, 0x01,
    // CborSchemaVersion
    0x03, 0x01,
    // DeviceSerial
    0x07,
    0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
    // SoftwareType
    0x0a,
    0x64, 'm', 'a', 'i', 'n',
    // SoftwareVersion
    0x09,
    0x65, '1' ,'.', '2', '.', '3',
    // HardwareVersion
    0x06,
    0x66, 'e', 'v', 't', '_', '2','4',
  };

  MEMCMP_EQUAL(expected_result, result, sizeof(result));
}
