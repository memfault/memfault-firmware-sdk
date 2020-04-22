#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "fakes/fake_memfault_event_storage.h"
#include "memfault/core/arch.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/trace_event.h"
#include "memfault_trace_event_private.h"

bool memfault_arch_is_inside_isr(void) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(false);
}

static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;

#define MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES (57)

TEST_GROUP(MfltTraceEvent) {
  void setup() {
    static uint8_t s_storage[100];
    fake_memfault_event_storage_clear();
    s_fake_event_storage_impl = memfault_events_storage_boot(
        &s_storage, sizeof(s_storage));

    mock().strictOrder();
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


static void prv_run_capture_test(void *pc, void *lr, size_t storage_size, size_t expected_encoded_size) {
  fake_memfault_event_storage_clear();
  fake_memfault_event_storage_set_available_space(storage_size);
  mock().expectOneCall("memfault_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  const bool expect_rollback = (storage_size < expected_encoded_size);
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  const int rv = memfault_trace_event_capture(pc, lr, kMfltTraceReasonUser_test);
  CHECK_EQUAL(expect_rollback ? -2 : 0, rv);
  mock().checkExpectations();
}

TEST(MfltTraceEvent, Test_CaptureOk_PcAndLr) {
  fake_memfault_event_storage_clear();
  const int rv = memfault_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
      0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
      0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
  };

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    void *pc = (void *)0x12345678;
    void *lr = (void *)0xaabbccdd;
    prv_run_capture_test(pc, lr, i, sizeof(expected_data));
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

static void prv_setup_isr_test(void) {
  fake_memfault_event_storage_clear();
  int rv = memfault_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  mock().expectOneCall("memfault_arch_is_inside_isr").andReturnValue(true);
  void *pc = (void *)0x12345678;
  void *lr = (void *)0xaabbccdd;
  rv = memfault_trace_event_capture(pc, lr, kMfltTraceReasonUser_test);
  CHECK_EQUAL(0, rv);
  mock().checkExpectations();

  mock().expectOneCall("memfault_arch_is_inside_isr").andReturnValue(true);
  void *pc2 = (void *)2;
  void *lr2 = (void *)1;
  rv = memfault_trace_event_capture(pc2, lr2, kMfltTraceReasonUser_test);
  CHECK_EQUAL(-2, rv);
  mock().checkExpectations();
}

TEST(MfltTraceEvent, Test_CaptureOk_FromIsrWithForceFlush) {
  prv_setup_isr_test();

  mock().expectOneCall("prv_begin_write");
  const bool expect_rollback = false;
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  const int rv = memfault_trace_event_try_flush_isr_event();
  CHECK_EQUAL(0, rv);
  mock().checkExpectations();

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

TEST(MfltTraceEvent, Test_CaptureOk_FromIsrWithLazyFlush) {
  prv_setup_isr_test();

  bool expect_rollback = true;
  fake_memfault_event_storage_set_available_space(10);
  mock().expectOneCall("memfault_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  void *pc2 = (void *)NULL;
  void *lr2 = (void *)1;
  int rv = memfault_trace_event_capture(pc2, lr2, kMfltTraceReasonUser_test);
  CHECK_EQUAL(-2, rv);
  mock().checkExpectations();

  const uint8_t expected_data[] = {
    // event logged from ISR
    0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
    0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
    0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
    0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
    0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
    0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
    0x1A, 0xAA, 0xBB, 0xCC, 0xDD,

    // event not logged from ISR
    0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
    0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
    0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
    0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
    0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA2, 0x06,
    0x03, 0x03, 0x01,
  };

  fake_memfault_event_storage_set_available_space(sizeof(expected_data));

  expect_rollback = false;
  // first the ISR event should be flushed now that we aren't in an ISR
  mock().expectOneCall("memfault_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);

  // then the second event we want to record should be stored
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  rv = memfault_trace_event_capture(pc2, lr2, kMfltTraceReasonUser_test);
  CHECK_EQUAL(0, rv);

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(MfltTraceEvent, Test_CaptureOk_LrOnly) {
  fake_memfault_event_storage_clear();
  const int rv = memfault_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA2, 0x06,
      0x03, 0x03, 0x01,
  };

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    void *pc = (void *)NULL;
    void *lr = (void *)1;
    prv_run_capture_test(pc, lr, i, sizeof(expected_data));
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(MfltTraceEvent, Test_CaptureStorageFull) {
  fake_memfault_event_storage_clear();
  fake_memfault_event_storage_set_available_space(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES);

  CHECK_EQUAL(memfault_trace_event_boot(s_fake_event_storage_impl), 0);
  fake_memfault_event_storage_set_available_space(0);
  mock().expectOneCall("memfault_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");

  // Expect rollback!
  mock().expectOneCall("prv_finish_write").withParameter("rollback", true);

  const int rv = memfault_trace_event_capture(0, 0, kMfltTraceReasonUser_test);
  CHECK_EQUAL(-2, rv);
}

TEST(MfltTraceEvent, Test_GetWorstCaseSerializeSize) {
  const size_t worst_case_size = memfault_trace_event_compute_worst_case_storage_size();
  LONGS_EQUAL(MEMFAULT_TRACE_EVENT_WORST_CASE_SIZE_BYTES, worst_case_size);
}
