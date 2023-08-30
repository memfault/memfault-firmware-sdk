//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Adds a basic set of commands for interacting with Memfault SDK

#include <stdlib.h>

#include "memfault/components.h"
#ifdef SL_COMPONENT_CATALOG_PRESENT
  #include "sl_component_catalog.h"
#endif  // SL_COMPONENT_CATALOG_PRESENT
#ifdef SL_CATALOG_CLI_PRESENT
  #include "sl_cli.h"
#endif  // SL_CATALOG_CLI_PRESENT

#ifdef SL_CATALOG_CLI_PRESENT

static void prv_adjust_arg_vars(sl_cli_command_arg_t *arguments, int *argc, void ***argv) {
  *argc = arguments->argc;
  *argv = arguments->argv;
  if (arguments->arg_ofs) {
    *argc = arguments->argc - arguments->arg_ofs + 1;
    *argv = &arguments->argv[arguments->arg_ofs + 1];
  }
}

void memfault_emlib_cli_clear_core(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_clear_core(argc, (char **)argv);
}

void memfault_emlib_cli_get_core(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_get_core(argc, (char **)argv);
}

void memfault_emlib_cli_logs(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_test_log(argc, (char **)argv);
}

void memfault_emlib_cli_log_capture(sl_cli_command_arg_t *arguments) {
  (void)arguments;
  memfault_demo_cli_cmd_trigger_logs(0, NULL);
}

void memfault_emlib_cli_get_device_info(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_get_device_info(argc, (char **)argv);
}

void memfault_emlib_cli_export(sl_cli_command_arg_t *arguments) {
  (void)arguments;
  memfault_data_export_dump_chunks();
}

void memfault_emlib_cli_trace(sl_cli_command_arg_t *arguments) {
  // For more information on user-defined error reasons, see
  // the MEMFAULT_TRACE_REASON_DEFINE macro in trace_reason_user.h .
  MEMFAULT_TRACE_EVENT_WITH_LOG(MemfaultCli_Test, "Num args: %d", arguments->argc);
  MEMFAULT_LOG_DEBUG("Trace Event Generated!");
}

void memfault_emlib_cli_heartbeat(sl_cli_command_arg_t *arguments) {
  (void)arguments;
  memfault_metrics_heartbeat_debug_trigger();
}

void memfault_emlib_cli_reboot(sl_cli_command_arg_t *arguments) {
  (void)arguments;
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
}

void memfault_emlib_cli_assert(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_assert(argc, (char **)argv);
}

void memfault_emlib_cli_hang(sl_cli_command_arg_t *arguments) {
  (void)arguments;
  MEMFAULT_LOG_INFO("Hanging system and waiting for watchdog!");
  while (1) {
  }
}

void memfault_emlib_cli_busfault(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_busfault(argc, (char **)argv);
}

void memfault_emlib_cli_hardfault(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_hardfault(argc, (char **)argv);
}

void memfault_emlib_cli_usagefault(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_usagefault(argc, (char **)argv);
}

void memfault_emlib_cli_memmanage(sl_cli_command_arg_t *arguments) {
  int argc = 0;
  void **argv = NULL;
  prv_adjust_arg_vars(arguments, &argc, &argv);
  memfault_demo_cli_cmd_memmanage(argc, (char **)argv);
}
#endif  // SL_CATALOG_CLI_PRESENT
