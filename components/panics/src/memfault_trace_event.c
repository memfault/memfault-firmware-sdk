//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/panics/trace_event.h"
#include "memfault_trace_event_private.h"

#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/serializer_helper.h"

#include <stdbool.h>
#include <stdint.h>

#define MEMFAULT_TRACE_EVENT_STORAGE_UNINITIALIZED (-1)
#define MEMFAULT_TRACE_EVENT_STORAGE_OUT_OF_SPACE (-2)
#define MEMFAULT_TRACE_EVENT_STORAGE_TOO_SMALL (-3)
#define MEMFAULT_TRACE_EVENT_BAD_PARAM (-4)

static struct {
  const sMemfaultEventStorageImpl *storage_impl;
} s_memfault_trace_event_ctx;

int memfault_trace_event_boot(const sMemfaultEventStorageImpl *storage_impl) {
  if (storage_impl == NULL) {
    return MEMFAULT_TRACE_EVENT_BAD_PARAM;
  }

  if (!memfault_serializer_helper_check_storage_size(
      storage_impl, memfault_trace_event_compute_worst_case_storage_size, "trace")) {
    return MEMFAULT_TRACE_EVENT_STORAGE_TOO_SMALL;
  }

  s_memfault_trace_event_ctx.storage_impl = storage_impl;
  return 0;
}

static bool prv_encode_cb(sMemfaultCborEncoder *encoder, void *ctx) {
  const sMemfaultTraceEventHelperInfo *info = (const sMemfaultTraceEventHelperInfo *)ctx;
  return memfault_serializer_helper_encode_trace_event(encoder, info);
}

int memfault_trace_event_capture(void *pc_addr, void *return_addr, eMfltTraceReasonUser reason) {
  if (s_memfault_trace_event_ctx.storage_impl == NULL) {
    return MEMFAULT_TRACE_EVENT_STORAGE_UNINITIALIZED;
  }

  sMemfaultTraceEventHelperInfo info = {
      .reason_key = kMemfaultTraceInfoEventKey_UserReason,
      .reason_value = reason,
      .pc = (uint32_t)(uintptr_t)pc_addr,
      .lr = (uint32_t)(uintptr_t)return_addr,
      .extra_event_info_pairs = 0,
  };
  sMemfaultCborEncoder encoder = { 0 };
  const bool success = memfault_serializer_helper_encode_to_storage(
      &encoder, s_memfault_trace_event_ctx.storage_impl, prv_encode_cb, &info);

  if (!success) {
    MEMFAULT_LOG_ERROR("%s storage out of space", __func__);
    return MEMFAULT_TRACE_EVENT_STORAGE_OUT_OF_SPACE;
  }

  return 0;
}

size_t memfault_trace_event_compute_worst_case_storage_size(void) {
  sMemfaultTraceEventHelperInfo info = {
      .reason_key = kMemfaultTraceInfoEventKey_UserReason,
      .reason_value = UINT32_MAX,
      .lr = UINT32_MAX,
      .pc = UINT32_MAX,
  };
  sMemfaultCborEncoder encoder = { 0 };
  return memfault_serializer_helper_compute_size(&encoder, prv_encode_cb, &info);
}

void memfault_trace_event_reset(void) {
  s_memfault_trace_event_ctx.storage_impl = NULL;
}
