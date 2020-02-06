#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "fakes/fake_memfault_event_storage.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/event_storage.h"
#include "memfault/panics/trace_event.h"
#include "memfault_trace_event_private.h"

static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;

#define MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES (57)

TEST_GROUP(MfltTraceEvent) {
  void setup() {
    static uint8_t s_storage[100];
    fake_memfault_event_storage_clear();
    s_fake_event_storage_impl = memfault_events_storage_boot(
        &s_storage, sizeof(s_storage));
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
    memfault_trace_event_reset();
  }
};

TEST(MfltTraceEvent, Test_BootNullStorage) {
  const int rv = memfault_trace_event_boot(NULL);
  CHECK_EQUAL(rv, -4);
}

TEST(MfltTraceEvent, Test_BootStorageTooSmall) {
  fake_memfault_event_storage_set_available_space(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES - 1);
  const int rv = memfault_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(rv, -3);
}


TEST(MfltTraceEvent, Test_CaptureButStorageUninitialized) {
  const int rv = memfault_trace_event_capture(NULL, NULL, kMfltTraceReasonUser_Unknown);
  CHECK_EQUAL(rv, -1);
}

TEST(MfltTraceEvent, Test_CaptureOk) {
  fake_memfault_event_storage_clear();
  fake_memfault_event_storage_set_available_space(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES);

  CHECK_EQUAL(memfault_trace_event_boot(s_fake_event_storage_impl), 0);

  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);

  void *pc = (void *)0x12345678;
  void *lr = (void *)0xaabbccdd;
  const int rv = memfault_trace_event_capture(pc, lr, kMfltTraceReasonUser_test);
  CHECK_EQUAL(rv, 0);

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
      0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
      0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
  };
  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(MfltTraceEvent, Test_CaptureStorageFull) {
  fake_memfault_event_storage_clear();
  fake_memfault_event_storage_set_available_space(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES);

  CHECK_EQUAL(memfault_trace_event_boot(s_fake_event_storage_impl), 0);
  fake_memfault_event_storage_set_available_space(0);
  mock().expectOneCall("prv_begin_write");

  // Expect rollback!
  mock().expectOneCall("prv_finish_write").withParameter("rollback", true);

  const int rv = memfault_trace_event_capture(0, 0, kMfltTraceReasonUser_test);
  CHECK_EQUAL(rv, -2);
}

TEST(MfltTraceEvent, Test_GetWorstCaseSerializeSize) {
  const size_t worst_case_size = memfault_trace_event_compute_worst_case_storage_size();
  LONGS_EQUAL(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES, worst_case_size);
}
