//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! CLI command that exercises the MEMFAULT_TRACE_EVENT API, capturing a
//! Trace Event with the error reason set to "MemfaultDemoCli_Error".

#include "memfault/core/trace_event.h"
#include "memfault/demo/cli.h"

int memfault_demo_cli_cmd_trace_event_capture(int argc, char *argv[]) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
  if (argc < 2) {
    MEMFAULT_TRACE_EVENT_WITH_LOG(MemfaultCli_Test, "Example Trace Event. Num Args %d", argc);
  } else {
    MEMFAULT_TRACE_EVENT_WITH_LOG(MemfaultCli_Test, "%s", argv[1]);
  }
  return 0;
}
