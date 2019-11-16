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

  #include "fakes/fake_memfault_platform_metrics_locking.h"
  #include "memfault/core/data_packetizer_source.h"
  #include "memfault/core/event_storage.h"
  #include "memfault/core/event_storage_implementation.h"

  static uint8_t s_ram_store[11];
  static const size_t s_ram_store_size = sizeof(s_ram_store);
  #define MEMFAULT_STORAGE_OVERHEAD 2
  static const sMemfaultEventStorageImpl *s_storage_impl;
}

static bool prv_fake_event_impl_has_event(size_t *total_size) {
  return g_memfault_event_data_source.has_more_msgs_cb(total_size);
}

static bool prv_fake_event_impl_read(uint32_t offset, void *buf, size_t buf_len) {
  return g_memfault_event_data_source.read_msg_cb(offset, buf, buf_len);
}

static void prv_fake_event_impl_mark_event_read(void) {
  g_memfault_event_data_source.mark_msg_read_cb();
}


TEST_GROUP(MemfaultEventStorage) {
  void setup() {
    fake_memfault_metrics_platorm_locking_reboot();
    s_storage_impl = memfault_events_storage_boot(s_ram_store, s_ram_store_size);
    LONGS_EQUAL(s_ram_store_size, s_storage_impl->get_storage_size_cb());
  }
  void teardown() {
    CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MemfaultEventStorage, Test_MemfaultMetricStoreSingleEvent) {
  size_t space_available = s_storage_impl->begin_write_cb();
  LONGS_EQUAL(s_ram_store_size - MEMFAULT_STORAGE_OVERHEAD, space_available);

  // if we begin another transaction while one is in progress, no space
  // should be available
  space_available = s_storage_impl->begin_write_cb();
  LONGS_EQUAL(0, space_available);

  const uint8_t payload[] = { 0x1, 0x2, 0x3, 0x4 };
  s_storage_impl->append_data_cb(&payload, sizeof(payload));

  // complete the transaction
  const bool rollback = false;
  s_storage_impl->finish_write_cb(rollback);

  // should be a no-op
  s_storage_impl->finish_write_cb(rollback);

  size_t total_size;
  bool has_event = prv_fake_event_impl_has_event(&total_size);
  CHECK(has_event);
  LONGS_EQUAL(sizeof(payload), total_size);

  // drain the active event
  prv_fake_event_impl_mark_event_read();

  // should be a no-op
  prv_fake_event_impl_mark_event_read();

  // should be no more events
  has_event = prv_fake_event_impl_has_event(&total_size);
  CHECK(!has_event);
  LONGS_EQUAL(0, total_size);
}

static void prv_write_payload(const void *data, size_t data_len, bool rollback) {
  size_t space_available = s_storage_impl->begin_write_cb();
  CHECK(space_available != 0);

  const uint8_t *byte = (const uint8_t *)data;
  for (size_t i = 0; i < data_len; i++) {
    s_storage_impl->append_data_cb(&byte[i], sizeof(byte[i]));
  }

  s_storage_impl->finish_write_cb(rollback);
}


TEST(MemfaultEventStorage, Test_MemfaultMultiEvent) {
  // queue up 3 one byte events which due to 2-byte overhead should take up 9 bytes
  bool rollback = false;
  size_t space_available;
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t byte = i;
    prv_write_payload(&byte, sizeof(byte), rollback);
  }

  // start a new event, only header should fit so space available should be 0
  space_available = s_storage_impl->begin_write_cb();
  LONGS_EQUAL(0, space_available);

  // drain events
  bool has_event;
  size_t event_size;
  for (int i = 0; i < 3; i++) {
    has_event = prv_fake_event_impl_has_event(&event_size);
    CHECK(has_event);
    LONGS_EQUAL(1, event_size);

    // try an illegal read
    uint8_t data;
    bool success = prv_fake_event_impl_read(1, &data, sizeof(data));
    CHECK(!success);

    // read the data
    success = prv_fake_event_impl_read(0, &data, sizeof(data));
    CHECK(success);
    LONGS_EQUAL((uint8_t)i, data);

    prv_fake_event_impl_mark_event_read();
  }

  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(!has_event);

  // abort the write we had in progress and restart it
  rollback = true;
  s_storage_impl->finish_write_cb(true);

  // now write a larger message 1 byte at a time, all 11 bytes of storage should be free
  // abort the first attempt and then actually do the write on the second attempt
  const uint8_t payload[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8 };
  for (int i = 0; i < 2; i++) {
    rollback = (i == 0);
    prv_write_payload(&payload, sizeof(payload), rollback);
  }

  // new event but header can't fit, so space available should be zero
  space_available = s_storage_impl->begin_write_cb();
  LONGS_EQUAL(0, space_available);

  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(has_event);
  LONGS_EQUAL(sizeof(payload), event_size);

  uint8_t result[event_size];
  memset(result, 0x0, sizeof(result));
  for (uint32_t i = 0; i < sizeof(payload); i++) {
    prv_fake_event_impl_read(i, &result[i], sizeof(result[i]));
  }

  MEMCMP_EQUAL(payload, result, sizeof(result));
  prv_fake_event_impl_mark_event_read();
}
