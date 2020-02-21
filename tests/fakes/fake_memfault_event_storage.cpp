//! @file
//!
//! @brief
//! A very simple fake storage implementation

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "fakes/fake_memfault_event_storage.h"

#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"

typedef struct FakeEventStorageState {
  uint8_t *buf;
  size_t total_size;
  size_t space_available;
  size_t curr_offset;
  size_t start_offset;
} sFakeEventStorageState;

static sFakeEventStorageState s_event_storage_state;

static size_t prv_begin_write(void) {
  mock().actualCall(__func__);
  s_event_storage_state.start_offset = s_event_storage_state.curr_offset;
  uint8_t *startp = &s_event_storage_state.buf[s_event_storage_state.curr_offset];
  const size_t total_size = s_event_storage_state.total_size - s_event_storage_state.curr_offset;

  memset(startp, 0x0, total_size);
  return s_event_storage_state.space_available;
}

// offset not really needed by encoder
static bool prv_append_data(const void *bytes, size_t num_bytes) {
  const uint32_t offset = s_event_storage_state.curr_offset;
  CHECK((offset + num_bytes) <= s_event_storage_state.space_available);

  uint8_t *buf = s_event_storage_state.buf;
  memcpy(&buf[offset], bytes, num_bytes);
  s_event_storage_state.curr_offset += num_bytes;
  return true;
}

static void prv_finish_write(bool rollback) {
  if (rollback) {
    s_event_storage_state.curr_offset = s_event_storage_state.start_offset;
  }
  mock().actualCall(__func__).withParameter("rollback", rollback);
}

static size_t prv_get_size(void) {
  return s_event_storage_state.space_available;
}

void fake_event_storage_assert_contents_match(const void *buf, size_t buf_len) {
  LONGS_EQUAL(buf_len, s_event_storage_state.curr_offset);
  MEMCMP_EQUAL(buf, s_event_storage_state.buf, buf_len);
}

void fake_memfault_event_storage_clear(void) {
  s_event_storage_state.curr_offset = 0;
  s_event_storage_state.space_available = s_event_storage_state.total_size;
}

void fake_memfault_event_storage_set_available_space(size_t space_available) {
  CHECK(space_available < s_event_storage_state.total_size);
  s_event_storage_state.space_available = space_available;
}

const sMemfaultEventStorageImpl *memfault_events_storage_boot(void *buf, size_t buf_len) {
  s_event_storage_state = (sFakeEventStorageState) {
    .buf = (uint8_t *)buf,
    .total_size = buf_len,
    .space_available = buf_len,
    .curr_offset = 0,
  };

  static const sMemfaultEventStorageImpl s_fake_storage_impl = {
    .begin_write_cb = &prv_begin_write,
    .append_data_cb = &prv_append_data,
    .finish_write_cb = &prv_finish_write,
    .get_storage_size_cb = &prv_get_size,
  };
  return &s_fake_storage_impl;
}
