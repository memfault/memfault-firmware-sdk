//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/panics/trace_event.h"
#include "memfault_trace_event_private.h"

#include <stdbool.h>
#include <stdint.h>

#include "memfault/core/arch.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/serializer_helper.h"

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

typedef struct {
  uint32_t reason;
  void *pc_addr;
  void *return_addr;
} sMemfaultIsrTraceEvent;

static sMemfaultIsrTraceEvent s_isr_trace_event;

// To keep the number of cycles spent logging a trace from an ISR to a minimum we just copy the
// values into a storage area and then flush the data after the system has returned from an ISR
static int prv_trace_event_capture_from_isr(void *pc_addr, void *return_addr, eMfltTraceReasonUser reason) {
  uint32_t expected_reason = MEMFAULT_TRACE_REASON(Unknown);
  const uint32_t desired_reason = (uint32_t)reason;

  if (s_isr_trace_event.reason != expected_reason) {
    return MEMFAULT_TRACE_EVENT_STORAGE_OUT_OF_SPACE;
  }

  // NOTE: It's perfectly fine to be interrupted by a higher priority interrupt at this point.
  // In the unlikely scenario where that exception also logged a trace event we will just wind
  // up overwriting it. The actual update of the reason (32 bit write) is an atomic op
  s_isr_trace_event.reason = desired_reason;

  s_isr_trace_event.pc_addr = pc_addr;
  s_isr_trace_event.return_addr = return_addr;
  return 0;
}

static int prv_trace_event_capture(void *pc_addr, void *return_addr, eMfltTraceReasonUser reason) {
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

int memfault_trace_event_try_flush_isr_event(void) {
  if (s_isr_trace_event.reason == MEMFAULT_TRACE_REASON(Unknown)) {
    return 0;
  }

  const int rv = prv_trace_event_capture(s_isr_trace_event.pc_addr, s_isr_trace_event.return_addr,
                                         (eMfltTraceReasonUser)s_isr_trace_event.reason);
  if (rv == 0) {
    // we successfully flushed the ISR event, mark the space as free to use again
    s_isr_trace_event.reason = MEMFAULT_TRACE_REASON(Unknown);
  }
  return rv;
}

int memfault_trace_event_capture(void *pc_addr, void *return_addr, eMfltTraceReasonUser reason) {
  if (s_memfault_trace_event_ctx.storage_impl == NULL) {
    return MEMFAULT_TRACE_EVENT_STORAGE_UNINITIALIZED;
  }

  if (memfault_arch_is_inside_isr()) {
    return prv_trace_event_capture_from_isr(pc_addr, return_addr, reason);
  }

  // NOTE: We flush any ISR pended events here so that the order in which events are captured is
  // preserved. An user of the trace event API can also flush ISR events at anytime by explicitly
  // calling memfault_trace_event_try_flush_isr_event()
  const int rv = memfault_trace_event_try_flush_isr_event();
  if (rv != 0) {
    return rv;
  }

  return prv_trace_event_capture(pc_addr, return_addr, reason);
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
  s_isr_trace_event = (sMemfaultIsrTraceEvent) { 0 };
}
