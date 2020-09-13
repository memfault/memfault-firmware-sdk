#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs used internally by the SDK for trace event collection. An user
//! of the SDK should never have to call these routines directly.

#include "memfault/core/trace_reason_user.h"

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int memfault_trace_event_capture(eMfltTraceReasonUser reason, void *pc_addr, void *lr_addr);
int memfault_trace_event_with_status_capture(
    eMfltTraceReasonUser reason, void *pc_addr, void *lr_addr, int32_t status);

#ifdef __cplusplus
}
#endif
