//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Command definitions for the minimal shell/console implementation.

#include "memfault_demo_shell_commands.h"

#include "memfault/core/math.h"
#include "memfault/demo/cli.h"

#include <stddef.h>

static const sMemfaultShellCommand s_memfault_shell_commands[] = {
  {"get_core", memfault_demo_cli_cmd_get_core, "Get coredump info"},
  {"clear_core", memfault_demo_cli_cmd_clear_core, "Clear an existing coredump"},
  {"print_chunk", memfault_demo_cli_cmd_print_chunk, "Get next Memfault data chunk to send and print as a curl command"},
  {"crash", memfault_demo_cli_cmd_crash, "Trigger a crash"},
  {"drain_chunks",  memfault_demo_drain_chunk_data, "Flushes queued Memfault data. To upload data see https://mflt.io/posting-chunks-with-gdb"},
  {"trace", memfault_demo_cli_cmd_trace_event_capture, "Capture an example trace event"},
  {"get_device_info", memfault_demo_cli_cmd_get_device_info, "Get device info"},
  {"reboot", memfault_demo_cli_cmd_system_reboot, "Reboot system and tracks it with a trace event"},
  {"help", memfault_shell_help_handler, "Lists all commands"},
};

const sMemfaultShellCommand *const g_memfault_shell_commands = s_memfault_shell_commands;
const size_t g_memfault_num_shell_commands = MEMFAULT_ARRAY_SIZE(s_memfault_shell_commands);
