//! @file
//!
//! @brief

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_event_storage.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/serializer_helper.h"
#include "memfault/metrics/serializer.h"
#include "memfault/metrics/utils.h"

static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;
#define FAKE_EVENT_STORAGE_SIZE 58

// clang-format off
TEST_GROUP(MemfaultMetricsSerializer){
  void setup() {
     static uint8_t s_storage[FAKE_EVENT_STORAGE_SIZE];
     s_fake_event_storage_impl = memfault_events_storage_boot(
         &s_storage, sizeof(s_storage));
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};
// clang-format on

//
// For the purposes of our serialization test, we will
// just serialize 1 of each supported type, two unset
// integers (signed + unsigned), and a non-heartbeat
// session metric for a total of 7 metrics.
//

void memfault_metrics_heartbeat_iterate(MemfaultMetricIteratorCallback cb, void *ctx) {
    sMemfaultMetricInfo info = {0};
    // Note: info.key._impl is not needed for serialization so leaving blank

    info.session_key = MEMFAULT_METRICS_SESSION_KEY(heartbeat);
    info.type = kMemfaultMetricType_Unsigned;
    info.val.u32 = 1000;
    info.is_set = true;
    cb(ctx, &info);

    // Test a session metric
    info.session_key = MEMFAULT_METRICS_SESSION_KEY(test_key_session);
    cb(ctx, &info);

    // Test an unset unsigned metric
    info.session_key = MEMFAULT_METRICS_SESSION_KEY(heartbeat);
    info.is_set = false;
    cb(ctx, &info);

    info.type = kMemfaultMetricType_Signed;
    info.val.i32 = -1000;
    info.is_set = true;
    cb(ctx, &info);

    // Test an unset signed metric
    info.is_set = false;
    cb(ctx, &info);

    info.type = kMemfaultMetricType_Timer;
    info.val.u32 = 1234;
    cb(ctx, &info);

    info.type = kMemfaultMetricType_String;
// chosen to be exactly 16 bytes to match the max storage set in
// tests/stub_includes/memfault_metrics_heartbeat_config.def
#define SAMPLE_STRING "123456789abcde"
    uint8_t sample_string[sizeof(SAMPLE_STRING)];
    info.val.ptr = sample_string;
    memcpy(info.val.ptr, SAMPLE_STRING, sizeof(SAMPLE_STRING));
    cb(ctx, &info);
}

size_t memfault_metrics_session_get_num_metrics(eMfltMetricsSessionIndex session_key) {
    // if this fails, it means we need to add add a report for the new type
    // to the fake "memfault_metrics_heartbeat_iterate"
    LONGS_EQUAL(kMemfaultMetricType_NumTypes, 4);

    size_t num_metrics = 0;
    if (session_key == MEMFAULT_METRICS_SESSION_KEY(heartbeat)) {
      // Additionally we test that unset integers (signed + unsigned) are set to null
      num_metrics = kMemfaultMetricType_NumTypes + 2;
    } else {
      // We only have one session metric
      num_metrics = 1;
    }

    return num_metrics;
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricSerialize) {
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);

  memfault_metrics_heartbeat_serialize(s_fake_event_storage_impl);

  // {
  // "2": 1,
  // "3": 1,
  // "10": "main",
  // "9": "1.2.3",
  // "6": "evt_24",
  // "4": {
  //  "2": 1
  //  "1": [ 1000, null, -1000, null, 1234, "123456789abcde" ]
  //  }
  // }
  const uint8_t expected_serialization[] = {
    0xa6, 0x02, 0x01, 0x03, 0x01, 0x0a, 0x64, 'm',  'a',  'i',  'n',  0x09, 0x65, '1',  '.',
    '2',  '.',  '3',  0x06, 0x66, 'e',  'v',  't',  '_',  '2',  '4',  0x04, 0xa2, 0x02, 0x01,
    0x01, 0x86, 0x19, 0x03, 0xe8, 0xf6, 0x39, 0x03, 0xe7, 0xf6, 0x19, 0x04, 0xd2, 0x6e, '1',
    '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  'a',  'b',  'c',  'd',  'e'};

  fake_event_storage_assert_contents_match(expected_serialization, sizeof(expected_serialization));
}

TEST(MemfaultMetricsSerializer, Test_MemfaultSessionMetricSerialize) {
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);

  memfault_metrics_session_serialize(s_fake_event_storage_impl,
                                     MEMFAULT_METRICS_SESSION_KEY(test_key_session));

  // {
  // "2": 1,
  // "3": 1,
  // "10": "main",
  // "9": "1.2.3",
  // "6": "evt_24",
  // "4": {
  //  "2": 0
  //  "1": [ 1000 ]
  //  }
  // }
  const uint8_t expected_serialization[] = {0xa6, 0x02, 0x01, 0x03, 0x01, 0x0a, 0x64, 'm', 'a',
                                            'i',  'n',  0x09, 0x65, '1',  '.',  '2',  '.', '3',
                                            0x06, 0x66, 'e',  'v',  't',  '_',  '2',  '4', 0x04,
                                            0xa2, 0x02, 0x00, 0x01, 0x81, 0x19, 0x03, 0xE8};

  fake_event_storage_assert_contents_match(expected_serialization, sizeof(expected_serialization));
}

TEST(MemfaultMetricsSerializer, Test_MemfaultHeartbeatMetricSerializeWorstCaseSize) {
  const size_t worst_case_storage = memfault_metrics_heartbeat_compute_worst_case_storage_size();
  LONGS_EQUAL(72, worst_case_storage);
}

TEST(MemfaultMetricsSerializer, Test_MemfaultSessionMetricSerializeWorstCaseSize) {
  const size_t worst_case_storage = memfault_metrics_session_compute_worst_case_storage_size(
    MEMFAULT_METRICS_SESSION_KEY(test_key_session));
  LONGS_EQUAL(37, worst_case_storage);
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricSerializeOutOfSpace) {
  // iterate over all buffer sizes less than the encoding we need
  // this should exercise all early exit paths
  for (size_t i = 0; i < FAKE_EVENT_STORAGE_SIZE - 1; i++) {
    fake_memfault_event_storage_clear();
    fake_memfault_event_storage_set_available_space(i);

    mock().expectOneCall("prv_begin_write");
    mock().expectOneCall("prv_finish_write").withParameter("rollback", true);

    memfault_metrics_heartbeat_serialize(s_fake_event_storage_impl);

    mock().checkExpectations();
  }

  uint32_t drops = memfault_serializer_helper_read_drop_count();
  LONGS_EQUAL(FAKE_EVENT_STORAGE_SIZE - 1, drops);

  drops = memfault_serializer_helper_read_drop_count();
  LONGS_EQUAL(0, drops);
}

TEST(MemfaultMetricsSerializer, Test_MemfaultMetricTypes) {
  //! These should never change so that the same value can
  //! always be used to recover the type on the server
  CHECK_EQUAL(0, kMemfaultMetricType_Unsigned);
  CHECK_EQUAL(1, kMemfaultMetricType_Signed);
  CHECK_EQUAL(2, kMemfaultMetricType_Timer);
  CHECK_EQUAL(3, kMemfaultMetricType_String);
  //! This can change if new types are appended to the enum
  //! but we assert here to remind us to add the new type
  //! to the check here
  CHECK_EQUAL(4, kMemfaultMetricType_NumTypes);
}
