#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "fakes/fake_memfault_event_storage.h"
#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/core/serializer_helper.h"
#include "memfault_reboot_tracking_private.h"

static sMfltResetReasonInfo s_fake_reset_reason_info;
static const sMemfaultEventStorageImpl *s_fake_event_storage_impl;

bool memfault_reboot_tracking_read_reset_info(sMfltResetReasonInfo *info) {
  *info = s_fake_reset_reason_info;
  return true;
}

void memfault_reboot_tracking_clear_reset_info(void) {
  mock().actualCall(__func__);
}

TEST_GROUP(MfltRebootTrackingSerializer) {
  void setup() {
    static uint8_t s_storage[100];
    s_fake_event_storage_impl = memfault_events_storage_boot(
        &s_storage, sizeof(s_storage));
    s_fake_reset_reason_info = (sMfltResetReasonInfo) {
      .reason = kMfltRebootReason_Assert,
      .pc = 0xbadcafe,
      .lr = 0xdeadbeef,
      .reset_reason_reg0 = 0x12345678,
    };
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_run_reset_reason_serializer_test(const void *expected_data,
                                                 size_t expected_data_len) {
  fake_memfault_event_storage_clear();
  fake_memfault_event_storage_set_available_space(expected_data_len);

  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);
  mock().expectOneCall("memfault_reboot_tracking_clear_reset_info");
  const int rv = memfault_reboot_tracking_collect_reset_info(s_fake_event_storage_impl);
  LONGS_EQUAL(0, rv);

  fake_event_storage_assert_contents_match(expected_data, expected_data_len);
}

TEST(MfltRebootTrackingSerializer, Test_Serialize) {
  const uint8_t expected_data_all[] = {
    0xa6,
    0x02, 0x02,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65,'1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04, 0xa5,
    0x01, 0x19, 0x80, 0x01,
    0x02, 0x1a, 0x0b, 0xad, 0xca, 0xfe,
    0x03, 0x1a, 0xde, 0xad, 0xbe, 0xef,
    0x04, 0x1a, 0x12, 0x34, 0x56, 0x78,
    0x05, 0x00
  };
  // make sure short if there isn't enough storage, the write just fails
  for (size_t i = 0; i < sizeof(expected_data_all) - 1; i++) {
    fake_memfault_event_storage_clear();
    fake_memfault_event_storage_set_available_space(i);

    mock().expectOneCall("prv_begin_write");
    mock().expectOneCall("prv_finish_write").withParameter("rollback", true);
    const int rv = memfault_reboot_tracking_collect_reset_info(s_fake_event_storage_impl);
    LONGS_EQUAL(-2, rv);
  }

  prv_run_reset_reason_serializer_test(expected_data_all, sizeof(expected_data_all));

  s_fake_reset_reason_info.reset_reason_reg0 = 0;
  const uint8_t expected_data_pc_lr[] = {
    0xa6,
    0x02, 0x02,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65,'1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04, 0xa4,
    0x01, 0x19, 0x80, 0x01,
    0x02, 0x1a, 0x0b, 0xad, 0xca, 0xfe,
    0x03, 0x1a, 0xde, 0xad, 0xbe, 0xef,
    0x05, 0x00
  };
  prv_run_reset_reason_serializer_test(expected_data_pc_lr, sizeof(expected_data_pc_lr));

  s_fake_reset_reason_info.lr = 0;
  const uint8_t expected_data_pc[] = {
    0xa6,
    0x02, 0x02,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65,'1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04, 0xa3,
    0x01, 0x19, 0x80, 0x01,
    0x02, 0x1a, 0x0b, 0xad, 0xca, 0xfe,
    0x05, 0x00
  };
  prv_run_reset_reason_serializer_test(expected_data_pc, sizeof(expected_data_pc));

  s_fake_reset_reason_info.pc = 0;
  // indicate that there is a coredump to go along with it
  s_fake_reset_reason_info.coredump_saved = true;
  const uint8_t expected_data_no_optionals[] = {
    0xa6,
    0x02, 0x02,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65,'1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04, 0xa2,
    0x01, 0x19, 0x80, 0x01,
    0x05, 0x01
  };
  prv_run_reset_reason_serializer_test(expected_data_no_optionals,
                                       sizeof(expected_data_no_optionals));
}

TEST(MfltRebootTrackingSerializer, Test_GetWorstCaseSerializeSize) {
  const size_t worst_case_size = memfault_reboot_tracking_compute_worst_case_storage_size();
  LONGS_EQUAL(52, worst_case_size);
}

TEST(MfltRebootTrackingSerializer, Test_BadParams) {
  const int rv = memfault_reboot_tracking_collect_reset_info(NULL);
  LONGS_EQUAL(-1, rv);
}
