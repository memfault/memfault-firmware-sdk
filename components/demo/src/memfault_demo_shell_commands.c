//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Command definitions for the minimal shell/console implementation.

#include "memfault/demo/shell_commands.h"

#include <stddef.h>

#include "memfault/core/compiler.h"
#include "memfault/core/data_export.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/demo/cli.h"

static int prv_panics_component_required(void) {
  MEMFAULT_LOG_RAW("Disabled. panics component integration required");
  return -1;
}

MEMFAULT_WEAK
int memfault_demo_cli_cmd_get_core(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

MEMFAULT_WEAK
int memfault_demo_cli_cmd_clear_core(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

MEMFAULT_WEAK
int memfault_demo_cli_cmd_crash(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

int memfault_demo_cli_cmd_export(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  memfault_data_export_dump_chunks();

  return 0;
}

static const sMemfaultShellCommand s_memfault_shell_commands[] = {
  {"clear_core", memfault_demo_cli_cmd_clear_core, "Clear an existing coredump"},
  {"drain_chunks",  memfault_demo_drain_chunk_data, "Flushes queued Memfault data. To upload data see https://mflt.io/posting-chunks-with-gdb"},
  {"export", memfault_demo_cli_cmd_export, "Export base64-encoded chunks. To upload data see https://mflt.io/chunk-data-export"},
  {"get_core", memfault_demo_cli_cmd_get_core, "Get coredump info"},
  {"get_device_info", memfault_demo_cli_cmd_get_device_info, "Get device info"},

  //
  // Test commands for validating SDK functionality: https://mflt.io/mcu-test-commands
  //

  {"test_assert",  memfault_demo_cli_cmd_assert, "Trigger memfault assert"},

#if MEMFAULT_COMPILER_ARM
  {"test_busfault",  memfault_demo_cli_cmd_busfault, "Trigger a busfault"},
  {"test_hardfault",  memfault_demo_cli_cmd_hardfault, "Trigger a hardfault"},
  {"test_memmanage",  memfault_demo_cli_cmd_memmanage, "Trigger a memory management fault"},
  {"test_usagefault",  memfault_demo_cli_cmd_usagefault, "Trigger a usage fault"},
#endif

  {"test_log", memfault_demo_cli_cmd_test_log, "Writes test logs to log buffer"},
  {"test_log_capture",  memfault_demo_cli_cmd_trigger_logs, "Trigger capture of current log buffer contents"},
  {"test_reboot",  memfault_demo_cli_cmd_system_reboot, "Force system reset and track it with a trace event"},
  {"test_trace",  memfault_demo_cli_cmd_trace_event_capture, "Capture an example trace event"},

  {"help", memfault_shell_help_handler, "Lists all commands"},
};

// Note: Declared as weak so an end user can override the command table
MEMFAULT_WEAK
const sMemfaultShellCommand *const g_memfault_shell_commands = s_memfault_shell_commands;
MEMFAULT_WEAK
const size_t g_memfault_num_shell_commands = MEMFAULT_ARRAY_SIZE(s_memfault_shell_commands);
