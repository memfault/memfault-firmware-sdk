//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fakes/fake_memfault_platform_metrics_locking.h"
#include "memfault/core/batched_events.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/nonvolatile_event_storage.h"

extern "C" {
// Declaration for test function used to reset event storage
extern void memfault_event_storage_reset(void);
}

static uint8_t s_ram_store[11];
static const size_t s_ram_store_size = sizeof(s_ram_store);
static const sMemfaultEventStorageImpl *s_storage_impl;
#define MEMFAULT_STORAGE_OVERHEAD 2

static bool prv_fake_event_impl_has_event(size_t *total_size) {
  return g_memfault_event_data_source.has_more_msgs_cb(total_size);
}

static bool prv_fake_event_impl_read(uint32_t offset, void *buf, size_t buf_len) {
  return g_memfault_event_data_source.read_msg_cb(offset, buf, buf_len);
}

static void prv_fake_event_impl_mark_event_read(void) {
  g_memfault_event_data_source.mark_msg_read_cb();
}

static void prv_assert_read(void *expected_data, size_t data_len) {
  size_t total_size;
  bool success = prv_fake_event_impl_has_event(&total_size);
  CHECK(success);

  LONGS_EQUAL(data_len, total_size);

  uint8_t actual_data[data_len];
  success = prv_fake_event_impl_read(0, actual_data, sizeof(actual_data));
  CHECK(success);

  MEMCMP_EQUAL(expected_data, actual_data, data_len);

  prv_fake_event_impl_mark_event_read();
}

static void prv_assert_no_more_events(void) {
  size_t total_size = 0xab;
  const bool has_event = prv_fake_event_impl_has_event(&total_size);
  CHECK(!has_event);
  LONGS_EQUAL(0, total_size);
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

static void prv_write_payload(const void *data, size_t data_len, bool rollback) {
  size_t space_available = s_storage_impl->begin_write_cb();
  CHECK(space_available != 0);

  const uint8_t *byte = (const uint8_t *)data;
  for (size_t i = 0; i < data_len; i++) {
    s_storage_impl->append_data_cb(&byte[i], sizeof(byte[i]));
  }

  s_storage_impl->finish_write_cb(rollback);
}

#if MEMFAULT_TEST_PERSISTENT_EVENT_STORAGE_DISABLE

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

  prv_assert_read((void *)&payload, sizeof(payload));

  // should be a no-op
  prv_fake_event_impl_mark_event_read();

  prv_assert_no_more_events();
}

TEST(MemfaultEventStorage, Test_ApiMisuse) {
  prv_assert_no_more_events();

  // if there are no events, we shouldn't be trying to read or clear one
  prv_fake_event_impl_mark_event_read();
  uint8_t c;
  const bool success = prv_fake_event_impl_read(0, &c, sizeof(c));
  CHECK(!success);
}

#if MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED == 0

TEST(MemfaultEventStorage, Test_MemfaultMultiEvent) {
  // queue up 3 one byte events which due to 2-byte overhead should take up 9 bytes
  bool rollback = false;
  size_t space_available;
  for (uint8_t byte = 0; byte < 3; byte++) {
    prv_write_payload(&byte, sizeof(byte), rollback);
  }

  // start a new event, only header should fit so space available should be 0
  space_available = s_storage_impl->begin_write_cb();
  LONGS_EQUAL(0, space_available);

  // drain events
  bool has_event;
  size_t event_size;
  for (uint8_t byte = 0; byte < 3; byte++) {
    has_event = prv_fake_event_impl_has_event(&event_size);
    CHECK(has_event);
    LONGS_EQUAL(1, event_size);

    // try an illegal read (past valid offset)
    uint8_t data;
    bool success = prv_fake_event_impl_read(1, &data, sizeof(data));
    CHECK(!success);

    success = prv_fake_event_impl_read(0, &data, sizeof(data));
    CHECK(success);
    LONGS_EQUAL(byte, data);
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

#else /* MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED */

void memfault_batched_events_build_header(
    size_t num_events, sMemfaultBatchedEventsHeader *header_out) {
  if (num_events <= 1) {
    return;
  }

  // a fake multi-byte header
  header_out->length = 2;
  header_out->data[0] = 0xAB;
  header_out->data[1] = num_events & 0xff;
}

TEST(MemfaultEventStorage, Test_MemfaultMultiEvent) {
  // queue up 2, 3 byte events which due to 2-byte overhead should take 10 bytes
  bool rollback = false;

  const uint8_t evt1[] = { 0x1, 0x2 };
  const uint8_t evt2[] = { 0x3, 0x4 };

  prv_write_payload(&evt1, sizeof(evt1), rollback);
  prv_write_payload(&evt2, sizeof(evt2), rollback);

  // drain events (all at once)
  bool has_event;
  size_t event_size;

  const uint8_t fake_header[] = { 0xAB, 0x02};

  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(has_event);
  LONGS_EQUAL(sizeof(fake_header) + sizeof(evt1) + sizeof(evt2), event_size);

  // try an illegal read (past valid offset)
  uint8_t data[event_size];
  memset(data, 0x0, sizeof(data));
  bool success = prv_fake_event_impl_read(8, data, sizeof(data));
  CHECK(!success);

  for (size_t i = 0; i < sizeof(data); i++) {
    success = prv_fake_event_impl_read(i, &data[i], 1);
    CHECK(success);
  }

  MEMCMP_EQUAL(fake_header, &data[0], sizeof(fake_header));
  MEMCMP_EQUAL(evt1, &data[sizeof(fake_header)], sizeof(evt1));
  MEMCMP_EQUAL(evt2, &data[sizeof(fake_header) + sizeof(evt1)], sizeof(evt2));

  prv_fake_event_impl_mark_event_read();

  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(!has_event);
}

// More bytes in event storage than MEMFAULT_EVENT_STORAGE_READ_BATCHING_MAX_BYTES (4)
TEST(MemfaultEventStorage, Test_MemfaultMultiEventLimited) {
  // queue up two events
  const uint8_t event1[] = { 0x1, 0x2 };
  const uint8_t event2[] = { 0x3 };
  const uint8_t event3[] = { 0x4, 0x5 };
  bool rollback = false;
  prv_write_payload(&event1, sizeof(event1), rollback);
  prv_write_payload(&event2, sizeof(event2), rollback);
  prv_write_payload(&event3, sizeof(event3), rollback);

  bool has_event;
  size_t event_size;
  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(has_event);
  LONGS_EQUAL(5, event_size);

  const uint8_t expected_data[] = { 0xAB, 0x02, 0x1, 0x2, 0x3 };
  uint8_t actual_data[sizeof(expected_data)];
  bool success = prv_fake_event_impl_read(0, actual_data, sizeof(actual_data));
  CHECK(success);
  MEMCMP_EQUAL(expected_data, actual_data, sizeof(actual_data));

  memset(actual_data, 0x0, sizeof(actual_data));
  // read 1 byte of header
  success = prv_fake_event_impl_read(0, &actual_data[0], 1);
  CHECK(success);
  // 1 byte header + 1 byte event
  success = prv_fake_event_impl_read(1, &actual_data[1], 2);
  CHECK(success);
  // rest of data - 1 byte event1 + 1 byte event2
  success = prv_fake_event_impl_read(3, &actual_data[3], 2);
  CHECK(success);

  MEMCMP_EQUAL(expected_data, actual_data, sizeof(actual_data));
  prv_fake_event_impl_mark_event_read();

  // final event
  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(has_event);
  LONGS_EQUAL(2, event_size);

  success = prv_fake_event_impl_read(0, actual_data, event_size);
  CHECK(success);
  MEMCMP_EQUAL(event3, actual_data, sizeof(event3));
  prv_fake_event_impl_mark_event_read();

  has_event = prv_fake_event_impl_has_event(&event_size);
  CHECK(!has_event);
  LONGS_EQUAL(0, event_size);
}

#endif /* MEMFAULT_EVENT_STORAGE_MAX_READ_BATCH_LEN */
//
// We use a compilation flag and run the test suite twice so we can test the default stub
// persistent source implementation as well as a real one
//

#else /* !MEMFAULT_TEST_PERSISTENT_EVENT_STORAGE_DISABLE */

typedef struct {
  const uint8_t *data;
  size_t len;
} sFakePersistedEvent;

const uint8_t test_event0[] = {
  0xa7,
  0x02, 0x01,
  0x03, 0x01,
  0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
};

const uint8_t test_event1[] = {
  0xa6,
  0x02, 0x01,
  0x03, 0x01,
  0x0a, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1' ,'.', '2', '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2','4',
};

typedef struct {
  size_t num_events;
  size_t events_read;
} sFakePersistentEventStorageState;

static sFakePersistentEventStorageState s_persistent_event_storage_state = { 0 };

static const sFakePersistedEvent s_fake_persisted_events[] = {
  {
    .data = &test_event0[0],
    .len = sizeof(test_event0)
  },
  {
    .data = &test_event1[0],
    .len = sizeof(test_event1)
  },
};

static bool prv_platform_nv_event_storage_read_has_event(size_t *event_size) {
  CHECK(s_persistent_event_storage_state.num_events <=
        MEMFAULT_ARRAY_SIZE(s_fake_persisted_events));

  // all events have been read!
  if (s_persistent_event_storage_state.num_events == s_persistent_event_storage_state.events_read) {
    *event_size = 0;
    return false;
  }

  *event_size = s_fake_persisted_events[s_persistent_event_storage_state.events_read].len;
  return true;
}

static bool prv_platform_nv_event_storage_read(uint32_t offset, void *buf, size_t buf_len) {
  CHECK(s_persistent_event_storage_state.num_events != 0);
  CHECK(s_persistent_event_storage_state.num_events <=
        MEMFAULT_ARRAY_SIZE(s_fake_persisted_events));
  const sFakePersistedEvent *event =
      &s_fake_persisted_events[s_persistent_event_storage_state.events_read];
  CHECK(offset + buf_len <= event->len);

  memcpy(buf, &event->data[offset], buf_len);
  return true;
}

static void prv_platform_nv_event_storage_consume(void) {
  s_persistent_event_storage_state.events_read++;
}

static int s_expected_write_payload_len = 1;

static bool prv_platform_nv_event_storage_write(
    MemfaultEventReadCallback event_read_cb, size_t total_size) {

  const bool success = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (!success) {
    return success;
  }

  LONGS_EQUAL(s_expected_write_payload_len, total_size);

  uint8_t expected_data[total_size];
  for (size_t i = 0; i < total_size; i++) {
    expected_data[i]= (uint8_t)i;
  }

  uint8_t actual_data[total_size];
  for (size_t i = 0; i < total_size; i++) {
    event_read_cb(i, &actual_data[i], 1);
  }
  MEMCMP_EQUAL(expected_data, actual_data, sizeof(actual_data));

  s_expected_write_payload_len++;
  return true;
}

void memfault_event_storage_request_persist_callback(
    const sMemfaultEventStoragePersistCbStatus *status) {
  const bool async_persist = mock().actualCall(__func__)
      .withParameter("volatile_storage_bytes_used", status->volatile_storage.bytes_used)
      .withParameter("volatile_storage_bytes_free", status->volatile_storage.bytes_free)
      .returnBoolValueOrDefault(true);
  if (async_persist) {
    return;
  }

  const int events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(1, events_persisted);
}

static void prv_write_and_expect_persist_callback(
    bool async_persist, const void *data, size_t data_len, bool rollback) {

  mock().expectOneCall("memfault_event_storage_request_persist_callback")
      .ignoreOtherParameters()
      .andReturnValue(async_persist);
  if (!async_persist) {
    s_expected_write_payload_len = data_len;
    mock().expectOneCall("prv_platform_nv_event_storage_write");
  }

  prv_write_payload(data, data_len, rollback);
  mock().checkExpectations();
}

static bool s_nv_storage_enabled = true;
static bool prv_platform_nv_event_storage_enabled(void) {
  return s_nv_storage_enabled;
}

const sMemfaultNonVolatileEventStorageImpl g_memfault_platform_nv_event_storage_impl = {
  .enabled = prv_platform_nv_event_storage_enabled,

  .has_event = prv_platform_nv_event_storage_read_has_event,
  .read = prv_platform_nv_event_storage_read,
  .consume = prv_platform_nv_event_storage_consume,

  .write = prv_platform_nv_event_storage_write,
};

TEST(MemfaultEventStorage, Test_ReadPersistedEvents) {
  const size_t num_persisted_events = MEMFAULT_ARRAY_SIZE(s_fake_persisted_events);
  s_persistent_event_storage_state = (sFakePersistentEventStorageState) {
    .num_events = num_persisted_events,
    .events_read = 0,
  };

  bool rollback = false;
  bool async = true;
  uint8_t byte = 0;

  for (size_t i = 0; i < num_persisted_events; i++) {
    if (i == 1) {
      prv_write_and_expect_persist_callback(async, &byte, sizeof(byte), rollback);
    }
    if (i >= 1) {
      const size_t bytes_used = sizeof(byte) + MEMFAULT_STORAGE_OVERHEAD;
      mock().expectOneCall("memfault_event_storage_request_persist_callback")
          .withParameter("volatile_storage_bytes_used", bytes_used)
          .withParameter("volatile_storage_bytes_free", s_ram_store_size - bytes_used);
    }

    const sFakePersistedEvent *event = &s_fake_persisted_events[i];
    prv_assert_read((void *)event->data, event->len);

    mock().checkExpectations();
  }

  // since the ram events have not been flushed yet
  prv_assert_no_more_events();
}

TEST(MemfaultEventStorage, Test_PersistStorageFatalError) {
  const size_t num_persisted_events = MEMFAULT_ARRAY_SIZE(s_fake_persisted_events);
  s_persistent_event_storage_state = (sFakePersistentEventStorageState) {
    .num_events = num_persisted_events,
    .events_read = 0,
  };

  // queue up one event in ram
  const bool rollback = false;
  const bool async = true;
  uint8_t two_bytes[] = { 0x0, 0x1 };
  prv_write_and_expect_persist_callback(async, &two_bytes, sizeof(two_bytes), rollback);

  const size_t bytes_used = sizeof(two_bytes) + MEMFAULT_STORAGE_OVERHEAD;
  mock().expectOneCall("memfault_event_storage_request_persist_callback")
      .withParameter("volatile_storage_bytes_used", bytes_used)
      .withParameter("volatile_storage_bytes_free", s_ram_store_size - bytes_used);

  const sFakePersistedEvent *event = &s_fake_persisted_events[0];
  prv_assert_read((void *)event->data, event->len);
  mock().checkExpectations();

  s_nv_storage_enabled = false;

  // should be a no-op if storage is disabled
  const int events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(0, events_persisted);

  // read should fail since nv storage went from enabled -> disabled
  uint8_t c;
  const bool success = prv_fake_event_impl_read(0, &c, sizeof(c));
  CHECK(!success);

  prv_assert_read((void *)two_bytes, sizeof(two_bytes));

  s_nv_storage_enabled = true;
}

TEST(MemfaultEventStorage, Test_PersistEvents) {
  int events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(0, events_persisted);

  uint8_t byte = 0;
  uint8_t two_bytes[] = { 0x0, 0x1 };

  bool rollback = false;
  bool async = true;

  prv_write_and_expect_persist_callback(!async, &byte, sizeof(byte), rollback);

  s_expected_write_payload_len = 1;
  prv_write_and_expect_persist_callback(async, &byte, sizeof(byte), rollback);
  prv_write_and_expect_persist_callback(async, &two_bytes, sizeof(two_bytes), rollback);


  mock().expectNCalls(2, "prv_platform_nv_event_storage_write");
  events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(2, events_persisted);
  mock().checkExpectations();

  s_expected_write_payload_len = 1;
  prv_write_and_expect_persist_callback(async, &byte, sizeof(byte), rollback);
  prv_write_and_expect_persist_callback(async, &two_bytes, sizeof(two_bytes), rollback);
  mock().expectOneCall("prv_platform_nv_event_storage_write");
  mock().expectOneCall("prv_platform_nv_event_storage_write").andReturnValue(false);
  events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(1, events_persisted);
  mock().checkExpectations();

  // should see one event get flushed
  mock().expectOneCall("prv_platform_nv_event_storage_write");
  events_persisted = memfault_event_storage_persist();
  LONGS_EQUAL(1, events_persisted);
  mock().checkExpectations();
}

TEST(MemfaultEventStorage, Test_UsedFreeSizes) {
  const size_t per_event_overhead = 2;
  const size_t first_half = s_ram_store_size / 2; // Integer truncation is fine
  const size_t second_half = s_ram_store_size - first_half;
  const bool async = true;
  const bool no_rollback = false;

  // We'll write the same buffer data twice.
  uint8_t my_buffer[sizeof s_ram_store / 2 + 1] = {0};

  // 1. Initially empty
  size_t bytes_used = memfault_event_storage_bytes_used();
  size_t bytes_free = memfault_event_storage_bytes_free();
  LONGS_EQUAL(bytes_used, 0);
  LONGS_EQUAL(bytes_free, s_ram_store_size);

  // NOTE: we write with knowledge of the event overhead but check
  // bytes used based on total bytes written to event storage.

  // 2. Partially full
  prv_write_and_expect_persist_callback(
      async, my_buffer, first_half-per_event_overhead, no_rollback);

  bytes_used = memfault_event_storage_bytes_used();
  bytes_free = memfault_event_storage_bytes_free();
  LONGS_EQUAL(bytes_used, first_half);
  LONGS_EQUAL(bytes_free, second_half);

  // 3. Full
  prv_write_and_expect_persist_callback(
      async, my_buffer, second_half-per_event_overhead, no_rollback);

  bytes_used = memfault_event_storage_bytes_used();
  bytes_free = memfault_event_storage_bytes_free();
  LONGS_EQUAL(bytes_used, s_ram_store_size);
  LONGS_EQUAL(bytes_free, 0);
}
#endif /* !MEMFAULT_TEST_PERSISTENT_EVENT_STORAGE_DISABLE */

TEST(MemfaultEventStorage, Test_EventStorageBoot) {
  uint8_t storage[11] = { 0 };
  const size_t storage_size = sizeof(storage);
  const sMemfaultEventStorageImpl *storage_impl = NULL;

  memfault_event_storage_reset();
  CHECK_FALSE(memfault_event_storage_booted());

  storage_impl = memfault_events_storage_boot(storage, storage_size);
  CHECK(storage_impl != NULL);
  CHECK(memfault_event_storage_booted());
}
