//! @file
//!
//! @brief
//! A RAM-backed storage API for serialized events. This is where events (such as heartbeats and
//! reset trace events) get stored as they wait to be chunked up and sent out over the transport.

#include "memfault/config.h"

#if defined(MEMFAULT_EVENT_STORAGE_LITE)

  #include <inttypes.h>
  #include <stdbool.h>
  #include <stdio.h>
  #include <string.h>

  #include "memfault/core/batched_events.h"
  #include "memfault/core/compiler.h"
  #include "memfault/core/data_packetizer_source.h"
  #include "memfault/core/debug_log.h"
  #include "memfault/core/event_storage.h"
  #include "memfault/core/event_storage_implementation.h"
  #include "memfault/core/math.h"
  #include "memfault/core/platform/nonvolatile_event_storage.h"
  #include "memfault/core/platform/overrides.h"
  #include "memfault/core/platform/system_time.h"
  #include "memfault/core/sdk_assert.h"
  #include "memfault/util/circular_buffer.h"

//
// Routines which can optionally be implemented.
// For more details see:
//  memfault/core/platform/system_time.h
//  memfault/core/platform/overrides.h
//  memfault/core/platform/event.h
//

MEMFAULT_WEAK bool memfault_platform_time_get_current(MEMFAULT_UNUSED sMemfaultCurrentTime *time) {
  return false;
}

MEMFAULT_WEAK void memfault_lock(void) { }

MEMFAULT_WEAK void memfault_unlock(void) { }

// Serialize the reboot event into non-circular storage; only support storing
// 1 event.
  #if !defined(MEMFAULT_EVENT_STORAGE_BUFFER_SIZE)
    #define MEMFAULT_EVENT_STORAGE_BUFFER_SIZE 128
  #endif
  #if !defined(MEMFAULT_EVENT_STORAGE_BUFFER_COUNT)
    #define MEMFAULT_EVENT_STORAGE_BUFFER_COUNT 2
  #endif
static struct MemfaultEventStorage {
  uint32_t write_active;
  uint32_t read_active;
  struct MemfaultEventStorageBuffer {
    size_t write_offset;
    uint8_t storage[MEMFAULT_EVENT_STORAGE_BUFFER_SIZE];
  } buffer[MEMFAULT_EVENT_STORAGE_BUFFER_COUNT];
} s_mflt_event_storage;

static size_t prv_event_storage_storage_begin_write(void) {
  struct MemfaultEventStorageBuffer *active_buffer =
    &s_mflt_event_storage.buffer[s_mflt_event_storage.write_active];

  if (active_buffer->write_offset != 0) {
    return 0;
  }

  active_buffer->write_offset = 0;
  return sizeof(active_buffer->storage);
}
static bool prv_event_storage_storage_append_data(const void *bytes, size_t num_bytes) {
  struct MemfaultEventStorageBuffer *active_buffer =
    &s_mflt_event_storage.buffer[s_mflt_event_storage.write_active];

  size_t offset = active_buffer->write_offset;
  if (offset + num_bytes > sizeof(active_buffer->storage)) {
    return false;
  }
  memcpy(&active_buffer->storage[offset], bytes, num_bytes);
  active_buffer->write_offset += num_bytes;
  return true;
}
static void prv_event_storage_storage_finish_write(bool rollback) {
  (void)rollback;
  s_mflt_event_storage.write_active =
    (s_mflt_event_storage.write_active + 1) % MEMFAULT_ARRAY_SIZE(s_mflt_event_storage.buffer);
}

static size_t prv_get_size_cb(void) {
  return sizeof(s_mflt_event_storage.buffer[s_mflt_event_storage.write_active].storage);
}

const sMemfaultEventStorageImpl *memfault_events_storage_boot(void *buf, size_t buf_len) {
  (void)buf, (void)buf_len;

  static const sMemfaultEventStorageImpl s_event_storage_impl = {
    .begin_write_cb = &prv_event_storage_storage_begin_write,
    .append_data_cb = &prv_event_storage_storage_append_data,
    .finish_write_cb = &prv_event_storage_storage_finish_write,
    .get_storage_size_cb = &prv_get_size_cb,
  };
  return &s_event_storage_impl;
}

static bool prv_has_event(size_t *event_size) {
  *event_size = s_mflt_event_storage.buffer[s_mflt_event_storage.read_active].write_offset;
  return *event_size > 0;
}

static bool prv_event_storage_read(uint32_t offset, void *buf, size_t buf_len) {
  struct MemfaultEventStorageBuffer *active_buffer =
    &s_mflt_event_storage.buffer[s_mflt_event_storage.read_active];
  if (offset + buf_len > active_buffer->write_offset) {
    return false;
  }
  memcpy(buf, &active_buffer->storage[offset], buf_len);
  return true;
}

static void prv_event_storage_mark_event_read(void) {
  // mark the buffer as free
  s_mflt_event_storage.buffer[s_mflt_event_storage.read_active].write_offset = 0;
  // rotate the read pointer
  s_mflt_event_storage.read_active =
    (s_mflt_event_storage.read_active + 1) % MEMFAULT_ARRAY_SIZE(s_mflt_event_storage.buffer);
}

//! Expose a data source for use by the Memfault Packetizer
const sMemfaultDataSourceImpl g_memfault_event_data_source = {
  .has_more_msgs_cb = prv_has_event,
  .read_msg_cb = prv_event_storage_read,
  .mark_msg_read_cb = prv_event_storage_mark_event_read,
};

// // These getters provide the information that user doesn't have. The user knows the total
// size
// // of the event storage because they supply it but they need help to get the free/used stats.
// size_t memfault_event_storage_bytes_used(void) {
//   size_t bytes_used;

//   memfault_lock();
//   { bytes_used = memfault_circular_buffer_get_read_size(&s_event_storage); }
//   memfault_unlock();

//   return bytes_used;
// }

// size_t memfault_event_storage_bytes_free(void) {
//   size_t bytes_free;

//   memfault_lock();
//   { bytes_free = memfault_circular_buffer_get_write_size(&s_event_storage); }
//   memfault_unlock();

//   return bytes_free;
// }

#endif  // MEMFAULT_EVENT_STORAGE_LITE
