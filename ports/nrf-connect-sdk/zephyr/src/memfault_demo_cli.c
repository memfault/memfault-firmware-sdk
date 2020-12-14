//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Adds a basic set of commands for interacting with Memfault SDK

#include "memfault/demo/cli.h"

#include <shell/shell.h>

#include "memfault/core/data_export.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/trace_event.h"
#include "memfault/nrfconnect_port/http.h"

static int prv_clear_core_cmd(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_clear_core(argc, argv);
}

static int prv_get_core_cmd(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_get_core(argc, argv);
}

static int prv_crash_example(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_crash(argc, argv);
}

static int prv_get_device_info(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_get_device_info(argc, argv);
}

static int prv_chunk_data_export(const struct shell *shell, size_t argc, char **argv) {
  memfault_data_export_dump_chunks();
  return 0;
}

static int prv_example_trace_event_capture(const struct shell *shell, size_t argc, char **argv) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
#if defined(MEMFAULT_TRACE_REASON_USER_DEFS_FILE)
  MEMFAULT_TRACE_EVENT(Unknown);
#else
  MEMFAULT_TRACE_EVENT(MemfaultDemoCli_Error);
#endif
  MEMFAULT_LOG_DEBUG("Trace Event Generated!");
  return 0;
}

static int prv_post_data(const struct shell *shell, size_t argc, char **argv) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
#if defined(CONFIG_MEMFAULT_SHELL)
  return memfault_nrfconnect_port_post_data();
#else
  return 0;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_memfault_cmds,
    SHELL_CMD(get_core, NULL, "gets the core", prv_get_core_cmd),
    SHELL_CMD(clear_core, NULL, "clear the core", prv_clear_core_cmd),
    SHELL_CMD(crash, NULL, "trigger a crash", prv_crash_example),
    SHELL_CMD(export_data, NULL, "dump chunks collected by Memfault SDK using https://mflt.io/chunk-data-export", prv_chunk_data_export),
    SHELL_CMD(trace, NULL, "Capture an example trace event", prv_example_trace_event_capture),
    SHELL_CMD(get_device_info, NULL, "display device information", prv_get_device_info),
    SHELL_CMD(post_chunks, NULL, "Post Memfault data to cloud", prv_post_data),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mflt, &sub_memfault_cmds, "Memfault Test Commands", NULL);
