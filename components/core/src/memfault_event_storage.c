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
#include <stdio.h>

#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/debug_log.h"
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

MEMFAULT_WEAK
void memfault_lock(void) { }

MEMFAULT_WEAK
void memfault_unlock(void) { }

MEMFAULT_WEAK
void memfault_event_storage_request_persist_callback(
    MEMFAULT_UNUSED const sMemfaultEventStoragePersistCbStatus *status) { }

static bool prv_nonvolatile_event_storage_enabled(void) {
  return false;
}

MEMFAULT_WEAK
const sMemfaultNonVolatileEventStorageImpl g_memfault_platform_nv_event_storage_impl = {
  .enabled = prv_nonvolatile_event_storage_enabled,
};

typedef struct {
  bool write_in_progress;
  size_t bytes_written;
} sMemfaultEventStorageWriteState;

typedef struct {
  size_t active_event_read_size;
} sMemfaultEventStorageReadState;

#define MEMFAULT_EVENT_STORAGE_WRITE_IN_PROGRESS 0xffff

typedef MEMFAULT_PACKED_STRUCT {
  uint16_t total_size;
} sMemfaultEventStorageHeader;

static sMfltCircularBuffer s_event_storage;
static sMemfaultEventStorageWriteState s_event_storage_write_state;
static sMemfaultEventStorageReadState s_event_storage_read_state;

static void prv_invoke_request_persist_callback(void) {
  sMemfaultEventStoragePersistCbStatus status;
  memfault_lock();
  {
    status = (sMemfaultEventStoragePersistCbStatus) {
      .volatile_storage = {
        .bytes_used = memfault_circular_buffer_get_read_size(&s_event_storage),
        .bytes_free = memfault_circular_buffer_get_write_size(&s_event_storage),
      },
    };
  }
  memfault_unlock();

  memfault_event_storage_request_persist_callback(&status);
}

static bool prv_has_event_ram(size_t *total_size) {
  sMemfaultEventStorageHeader hdr = { 0 };
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

  s_event_storage_read_state = (sMemfaultEventStorageReadState) {
    .active_event_read_size =  hdr.total_size,
  };

  *total_size = hdr.total_size - sizeof(hdr);
  return true;
}


static bool prv_event_storage_read_ram(uint32_t offset, void *buf, size_t buf_len) {
  offset += sizeof(sMemfaultEventStorageHeader);
  if (offset >= s_event_storage_read_state.active_event_read_size) {
    return false;
  }

  return memfault_circular_buffer_read(&s_event_storage, offset, buf, buf_len);
}

static void prv_event_storage_mark_event_read_ram(void) {
  if (s_event_storage_read_state.active_event_read_size == 0) {
    // no active event to clear
    return;
  }

  memfault_lock();
  {
    memfault_circular_buffer_consume(
        &s_event_storage, s_event_storage_read_state.active_event_read_size);
  }
  memfault_unlock();

  s_event_storage_read_state = (sMemfaultEventStorageReadState) { 0 };
}

// "begin" to write a heartbeat & return the space available
static size_t prv_event_storage_storage_begin_write(void) {
  if (s_event_storage_write_state.write_in_progress) {
    return 0;
  }

  const sMemfaultEventStorageHeader hdr = {
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

  s_event_storage_write_state = (sMemfaultEventStorageWriteState) {
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
      const sMemfaultEventStorageHeader hdr = {
        .total_size = (uint16_t)s_event_storage_write_state.bytes_written,
      };
      memfault_circular_buffer_write_at_offset(&s_event_storage,
                                               s_event_storage_write_state.bytes_written,
                                               &hdr, sizeof(hdr));
    }
  }
  memfault_unlock();

  // reset the write state
  s_event_storage_write_state = (sMemfaultEventStorageWriteState) { 0 };
  if (!rollback) {
    prv_invoke_request_persist_callback();
  }
}

static size_t prv_get_size_cb(void) {
  return memfault_circular_buffer_get_read_size(&s_event_storage) +
      memfault_circular_buffer_get_write_size(&s_event_storage);
}

const sMemfaultEventStorageImpl *memfault_events_storage_boot(void *buf, size_t buf_len) {
  memfault_circular_buffer_init(&s_event_storage, buf, buf_len);

  s_event_storage_write_state = (sMemfaultEventStorageWriteState) { 0 };
  s_event_storage_read_state = (sMemfaultEventStorageReadState) { 0 };

  static const sMemfaultEventStorageImpl s_event_storage_impl = {
    .begin_write_cb = &prv_event_storage_storage_begin_write,
    .append_data_cb = &prv_event_storage_storage_append_data,
    .finish_write_cb = &prv_event_storage_storage_finish_write,
    .get_storage_size_cb = &prv_get_size_cb,
  };
  return &s_event_storage_impl;
}

static bool prv_save_event_to_persistent_storage(void) {
  size_t total_size;
  if (!prv_has_event_ram(&total_size)) {
    return false;
  }

  const bool success = g_memfault_platform_nv_event_storage_impl.write(
      prv_event_storage_read_ram, total_size);
  if (success) {
    prv_event_storage_mark_event_read_ram();
  }
  return success;
}

static bool prv_nv_event_storage_enabled(void) {
  static bool s_nv_event_storage_enabled = false;

  MEMFAULT_SDK_ASSERT(g_memfault_platform_nv_event_storage_impl.enabled != NULL);
  const bool enabled = g_memfault_platform_nv_event_storage_impl.enabled();
  if (s_nv_event_storage_enabled && !enabled) {
    // This shouldn't happen and is indicative of a failure in nv storage. Let's reset the read
    // state in case we were in the middle of a read() trying to copy data into nv storage.
    s_event_storage_read_state = (sMemfaultEventStorageReadState) { 0 };
  }
  if (enabled) {
    // if nonvolatile storage is enabled, it is a configuration error if all the
    // required dependencies are not implemented!
    MEMFAULT_SDK_ASSERT(
        (g_memfault_platform_nv_event_storage_impl.has_event != NULL) &&
        (g_memfault_platform_nv_event_storage_impl.read != NULL) &&
        (g_memfault_platform_nv_event_storage_impl.consume != NULL) &&
        (g_memfault_platform_nv_event_storage_impl.write != NULL));
  }

  s_nv_event_storage_enabled = enabled;
  return s_nv_event_storage_enabled;
}

int memfault_event_storage_persist(void) {
  if (!prv_nv_event_storage_enabled()) {
    return 0;
  }

  int events_saved = 0;
  while (prv_save_event_to_persistent_storage()) {
    events_saved++;
  }

  return events_saved;
}

static void prv_nv_event_storage_mark_read_cb(void) {
  g_memfault_platform_nv_event_storage_impl.consume();

  size_t total_size;
  if (!prv_has_event_ram(&total_size)) {
    return;
  }

  prv_invoke_request_persist_callback();
}

static const sMemfaultDataSourceImpl *prv_get_active_event_storage_source(void) {
  static const sMemfaultDataSourceImpl s_memfault_ram_event_storage  = {
    .has_more_msgs_cb = prv_has_event_ram,
    .read_msg_cb = prv_event_storage_read_ram,
    .mark_msg_read_cb = prv_event_storage_mark_event_read_ram,
  };

  static sMemfaultDataSourceImpl s_memfault_nv_event_storage = { 0 };
  s_memfault_nv_event_storage = (sMemfaultDataSourceImpl) {
    .has_more_msgs_cb = g_memfault_platform_nv_event_storage_impl.has_event,
    .read_msg_cb = g_memfault_platform_nv_event_storage_impl.read,
    .mark_msg_read_cb = prv_nv_event_storage_mark_read_cb,
  };

  return prv_nv_event_storage_enabled() ? &s_memfault_nv_event_storage :
                                          &s_memfault_ram_event_storage;
}

static bool prv_has_event(size_t *event_size) {
  const sMemfaultDataSourceImpl *impl = prv_get_active_event_storage_source();
  return impl->has_more_msgs_cb(event_size);
}

static bool prv_event_storage_read(uint32_t offset, void *buf, size_t buf_len) {
  const sMemfaultDataSourceImpl *impl = prv_get_active_event_storage_source();
  return impl->read_msg_cb(offset, buf, buf_len);
}

static void prv_event_storage_mark_event_read(void) {
  const sMemfaultDataSourceImpl *impl = prv_get_active_event_storage_source();
  impl->mark_msg_read_cb();
}

//! Expose a data source for use by the Memfault Packetizer
const sMemfaultDataSourceImpl g_memfault_event_data_source  = {
  .has_more_msgs_cb = prv_has_event,
  .read_msg_cb = prv_event_storage_read,
  .mark_msg_read_cb = prv_event_storage_mark_event_read,
};
