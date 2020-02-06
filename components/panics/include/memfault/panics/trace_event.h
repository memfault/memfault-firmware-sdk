#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Subsystem to trace errors in a way that requires less storage than full coredump traces and
//! also allows the system to keep running after capturing the event. Only the program counter,
//! return address and a "reason" enum value are saved. Once uploaded to Memfault, each Trace Event
//! will be associated with an Issue.
//!
//! @note Where coredumps provide full backtraces of *all* threads in the system and local & global
//! variable values, trace events effectively only provide the 2 topmost frames of the backtrace of
//! the current thread only (no locals nor globals).
//!
//! @note For a step-by-step guide about how to integrate and leverage the Trace Event component
//! check out https://mflt.io/error-tracing

#include "memfault/core/event_storage.h"
#include "memfault/panics/trace_reason_user.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Sets the storage implementation used to record Trace Events.
//! @param storage_impl The event storage implementation being used (returned from
//!   memfault_events_storage_boot())
//! @return 0 on success, else error code.
//! @note This must be called before MEMFAULT_TRACE_EVENT / memfault_trace_event_capture can be
//! called!
int memfault_trace_event_boot(const sMemfaultEventStorageImpl *storage_impl);

//! Convenience macro to capture a Trace Event with given reason.
//!
//! The current program counter and return address are collected as part of the macro.
//!
//! @param reason The error reason. See MEMFAULT_TRACE_REASON_DEFINE in trace_reason_user.h for
//! information on how to define reason values.
//! @see memfault_trace_event_capture
//! @see MEMFAULT_TRACE_REASON_DEFINE
//! @note Ensure memfault_trace_event_boot() has been called before using this API!
#define MEMFAULT_TRACE_EVENT(reason)                                    \
do {                                                                    \
  void *pc;                                                             \
  MEMFAULT_GET_PC(pc);                                                  \
  void *lr;                                                             \
  MEMFAULT_GET_LR(lr);                                                  \
  memfault_trace_event_capture(pc, lr, MEMFAULT_TRACE_REASON(reason));  \
} while (0)

//! Captures a Trace Event with given program counter, return address and reason.
//!
//! This function should never be called directly by an end user. Instead, use the
//! MEMFAULT_TRACE_EVENT macro in your code which automatically collects the program counter and
//! return address.
//!
//! @param pc_addr The program counter
//! @param return_addr The return address
//! @param reason The error reason. See MEMFAULT_TRACE_REASON_DEFINE in trace_reason_user.h for
//! information on how to define reason values.
//! @return 0 on success, else error code.
//! @see MEMFAULT_TRACE_EVENT
//! @see MEMFAULT_TRACE_REASON_DEFINE
//! @note Ensure memfault_trace_event_boot() has been called before using this API!
int memfault_trace_event_capture(void *pc_addr, void *return_addr, eMfltTraceReasonUser reason);

//! Compute the worst case number of bytes required to serialize a Trace Event.
//!
//! @return the worst case amount of space needed to serialize a Trace Event.
size_t memfault_trace_event_compute_worst_case_storage_size(void);

#ifdef __cplusplus
}
#endif
