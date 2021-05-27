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

#include "fakes/fake_memfault_build_id.h"
#include "fakes/fake_memfault_platform_time.h"
#include "memfault/config.h"
#include "memfault/core/math.h"
#include "memfault/core/serializer_helper.h"
#include "memfault/util/cbor.h"

TEST_GROUP(MemfaultMetricsSerializerHelper){
  void setup() {
    fake_memfault_platform_time_enable(false);
    fake_memfault_build_id_reset();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
    fake_memfault_platform_time_enable(false);
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

static void prv_enable_captured_date(void) {
  fake_memfault_platform_time_enable(true);
  const sMemfaultCurrentTime time = {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 1586353908,
    }
  };
  fake_memfault_platform_time_set(&time);
}

static void prv_enable_build_id(void) {
  g_fake_memfault_build_id_type = kMemfaultBuildIdType_MemfaultBuildIdSha1;
}

typedef struct MemfaultSerializerHelperTestVector {
  const uint8_t *expected_encoding;
  const size_t expected_encoding_len;
  bool encode_captured_date;
  bool build_id_present;
} sMemfaultSerializerHelperTestVector;

#if MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL
// Optional Items: Device Serial
const uint8_t test_vector[] = {
  0xa7,
  0x02, 0x01,
  0x03, 0x01,
  0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
};

// Optional Items: Device Serial & Unix Epoch Timestamp
const uint8_t test_vector_with_timestamp[] = {
  0xa8,
  0x02, 0x01,
  0x03, 0x01,
  0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
  0x01, 0x1a, 0x5e, 0x8d, 0xd6, 0xf4,
};

// Optional Items: Build Id
const uint8_t test_vector_with_build_id[] = {
  0xa8,
  0x02, 0x01,
  0x03, 0x01,
  0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
  0x0B, 0x46,
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
};


#else

// Optional Items: None
const uint8_t test_vector[] = {
    0xa6,
    0x02, 0x01,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1' ,'.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2','4',
};

// Optional Items: Unix Epoch Timestamp
const uint8_t test_vector_with_timestamp[] = {
    0xa7,
    0x02, 0x01,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1' ,'.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2','4',
    0x01, 0x1a, 0x5e, 0x8d, 0xd6, 0xf4,
};

#if MEMFAULT_EVENT_INCLUDE_BUILD_ID
// Optional Items: Build Id
const uint8_t test_vector_with_build_id[] = {
  0xa7,
  0x02, 0x01,
  0x03, 0x01,
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
  0x0B, 0x46,
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
};
#else
// Optional Items: Build Id present, but MEMFAULT_EVENT_INCLUDE_BUILD_ID=0
const uint8_t test_vector_with_build_id[] = {
  0xa6,
  0x02, 0x01,
  0x03, 0x01,
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
};
#endif /* MEMFAULT_EVENT_INCLUDE_BUILD_ID */

#endif /* MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL */

// NB: Device Serial encoding is enabled/disabled at compile time based on the
// MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL. Encoding timestamp is optional based on whether or not
// memfault_platform_time_get_current is implemented. Let's run through all possible encoding combos.
const static sMemfaultSerializerHelperTestVector s_test_vectors[] = {
  {
    .expected_encoding = test_vector,
    .expected_encoding_len = sizeof(test_vector),
    .encode_captured_date = false,
    .build_id_present = false,
  },
  {
    .expected_encoding = test_vector_with_timestamp,
    .expected_encoding_len = sizeof(test_vector_with_timestamp),
    .encode_captured_date = true,
    .build_id_present = false,
  },
  {
    .expected_encoding = test_vector_with_build_id,
    .expected_encoding_len = sizeof(test_vector_with_build_id),
    .encode_captured_date = false,
    .build_id_present = true,
  },
};

TEST(MemfaultMetricsSerializerHelper, Test_MemfaultMetricSerializeEventMetadata) {
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_test_vectors); i++) {
    setup();

    const sMemfaultSerializerHelperTestVector *vec = &s_test_vectors[i];
    uint8_t result[vec->expected_encoding_len];
    memset(result, 0xa5, sizeof(result));

    if (vec->encode_captured_date) {
      prv_enable_captured_date();
    }

    if (vec->build_id_present) {
      prv_enable_build_id();
    }

    sMemfaultCborEncoder encoder;
    prv_encode_metadata_and_check(&encoder, result, sizeof(result));
    MEMCMP_EQUAL(vec->expected_encoding, result, sizeof(result));
  }
}
