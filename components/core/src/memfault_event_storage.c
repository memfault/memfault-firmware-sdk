//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A RAM-backed storage API for serialized events. This is where events (such as heartbeats and
//! reset trace events) get stored as they wait to be chunked up and sent out over the transport.

#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"

#include <stdbool.h>

#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/platform/overrides.h"
#include "memfault/core/platform/system_time.h"
#include "memfault/util/circular_buffer.h"

//
// Routines which can optionally me implemented.
// For more details see:
//  memfault/core/platform/system_time.h
//  memfault/core/platform/overrides.h
//

MEMFAULT_WEAK bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  return false;
}

MEMFAULT_WEAK
void memfault_lock(void) { }

MEMFAULT_WEAK
void memfault_unlock(void) { }

typedef struct {
  bool write_in_progress;
  size_t bytes_written;
} sHeartbeatStorageWriteState;

typedef struct {
  size_t active_event_read_size;
} sHeartbeatStorageReadState;

#define MEMFAULT_EVENT_STORAGE_WRITE_IN_PROGRESS 0xffff

typedef MEMFAULT_PACKED_STRUCT {
  uint16_t total_size;
} sHeartbeatStorageHeader;

static sMfltCircularBuffer s_event_storage;
static sHeartbeatStorageWriteState s_event_storage_write_state;
static sHeartbeatStorageReadState s_event_storage_read_state;

static bool prv_has_event(size_t *total_size) {
  sHeartbeatStorageHeader hdr = { 0 };
  bool success;
  memfault_lock();
  {
    success = memfault_circular_buffer_read(&s_event_storage, 0, &hdr, sizeof(hdr));
  }
  memfault_unlock();

  if (!success || hdr.total_size == MEMFAULT_EVENT_STORAGE_WRITE_IN_PROGRESS) {
    *total_size = 0;
    return false;
  }

  s_event_storage_read_state.active_event_read_size = hdr.total_size;
  *total_size = hdr.total_size - sizeof(hdr);
  return true;
}


static bool prv_event_storage_read(uint32_t offset, void *buf, size_t buf_len) {
  offset += sizeof(sHeartbeatStorageHeader);
  if (offset >= s_event_storage_read_state.active_event_read_size) {
    return false;
  }

  return memfault_circular_buffer_read(&s_event_storage, offset, buf, buf_len);
}

static void prv_event_storage_mark_event_read(void) {
  if (s_event_storage_read_state.active_event_read_size == 0) {
    // no active event to clear
    return;
  }
  memfault_lock();
  {
    memfault_circular_buffer_consume(&s_event_storage, s_event_storage_read_state.active_event_read_size);
  }
  memfault_unlock();
  s_event_storage_read_state = (sHeartbeatStorageReadState) { 0 };
}

// "begin" to write a heartbeat & return the space available
static size_t prv_event_storage_storage_begin_write(void) {
  if (s_event_storage_write_state.write_in_progress) {
    return 0;
  }

  const sHeartbeatStorageHeader hdr = {
    .total_size = MEMFAULT_EVENT_STORAGE_WRITE_IN_PROGRESS,
  };
  bool success;
  memfault_lock();
  {
    success = memfault_circular_buffer_write(&s_event_storage, &hdr, sizeof(hdr));
  }
  memfault_unlock();
  if (!success) {
    return 0;
  }

  s_event_storage_write_state = (sHeartbeatStorageWriteState) {
    .write_in_progress = true,
    .bytes_written = sizeof(hdr),
  };

  return memfault_circular_buffer_get_write_size(&s_event_storage);
}

static bool prv_event_storage_storage_append_data(const void *bytes, size_t num_bytes) {
  bool success;

  memfault_lock();
  {
    success = memfault_circular_buffer_write(&s_event_storage, bytes, num_bytes);
  }
  memfault_unlock();
  if (success) {
    s_event_storage_write_state.bytes_written += num_bytes;
  }
  return success;
}

static void prv_event_storage_storage_finish_write(bool rollback) {
  if (!s_event_storage_write_state.write_in_progress) {
    return;
  }

  memfault_lock();
  {
    if (rollback) {
      memfault_circular_buffer_consume_from_end(&s_event_storage,
                                                s_event_storage_write_state.bytes_written);
    } else {
      const sHeartbeatStorageHeader hdr = {
        .total_size = (uint16_t)s_event_storage_write_state.bytes_written,
      };
      memfault_circular_buffer_write_at_offset(&s_event_storage,
                                               s_event_storage_write_state.bytes_written,
                                               &hdr, sizeof(hdr));
    }
  }
  memfault_unlock();

  // reset the write state
  s_event_storage_write_state = (sHeartbeatStorageWriteState) { 0 };
}

static size_t prv_get_size_cb(void) {
  return memfault_circular_buffer_get_read_size(&s_event_storage) +
      memfault_circular_buffer_get_write_size(&s_event_storage);
}

const sMemfaultEventStorageImpl *memfault_events_storage_boot(void *buf, size_t buf_len) {
  memfault_circular_buffer_init(&s_event_storage, buf, buf_len);

  s_event_storage_write_state = (sHeartbeatStorageWriteState) { 0 };
  s_event_storage_read_state = (sHeartbeatStorageReadState) { 0 };

  static const sMemfaultEventStorageImpl s_event_storage_impl = {
    .begin_write_cb = &prv_event_storage_storage_begin_write,
    .append_data_cb = &prv_event_storage_storage_append_data,
    .finish_write_cb = &prv_event_storage_storage_finish_write,
    .get_storage_size_cb = &prv_get_size_cb,
  };
  return &s_event_storage_impl;
}

//! Expose a data source for use by the Memfault Packetizer
const sMemfaultDataSourceImpl g_memfault_event_data_source  = {
  .has_more_msgs_cb = prv_has_event,
  .read_msg_cb = prv_event_storage_read,
  .mark_msg_read_cb = prv_event_storage_mark_event_read,
};
