//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI implementation for Memfault NRF5 Demo application

#include "nrf.h"  // Includes CMSIS headers
#include "nrf_cli.h"
#include "nrf_cli_rtt.h"
#include "nrf_log.h"

#include "memfault/components.h"

// The nRF board is not capable of doing HTTP requests directly, but the 'print_chunk' command
// will use this information to print out a cURL command that you can copy & paste into a shell
// to post the chunk data to Memfault:

// Find your key on https://app.memfault.com/ under 'Settings':
sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "<YOUR PROJECT KEY HERE>",
};

NRF_CLI_RTT_DEF(cli_rtt);

// NB: Using '\n' as newline character so only works on OSX/Linux :)
NRF_CLI_DEF(m_cli, "rtt_cli:~$ ", &cli_rtt.transport,'\n', 4);

void mflt_cli_init(void) {
  APP_ERROR_CHECK(nrf_cli_init(&m_cli, NULL, false, false, NRF_LOG_SEVERITY_INFO));
  nrf_cli_start(&m_cli);
  m_cli.p_ctx->internal.flag.echo = 0;
}

void mflt_cli_try_process(void) {
  nrf_cli_process(&m_cli);
}

static void prv_clear_core_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_clear_core(argc, argv);
}

static void prv_get_core_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_get_core(argc, argv);
}

static void prv_crash_example(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_crash(argc, argv);
}

static void prv_test_log(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_test_log(argc, argv);
}

static void prv_trigger_logs(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_trigger_logs(argc, argv);
}

static void prv_trace_example(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_trace_event_capture(argc, argv);
}

static void prv_get_device_info(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_get_device_info(argc, argv);
}

static void prv_system_reboot_cmd(nrf_cli_t const *p_cli, size_t argc, char **argv) {
  memfault_demo_cli_cmd_system_reboot(argc, argv);
}

static void prv_export_data_cmd(nrf_cli_t const *p_cli, size_t argc, char **argv) {
  memfault_data_export_dump_chunks();
}

static void prv_coredump_storage_test_cmd(nrf_cli_t const *p_cli, size_t argc, char **argv) {
  __disable_irq();
  memfault_coredump_storage_debug_test_begin();
  __enable_irq();

  memfault_coredump_storage_debug_test_finish();
}

static void prv_hang_example(nrf_cli_t const *p_cli, size_t argc, char **argv) {
  MEMFAULT_LOG_DEBUG("Hanging system and waiting for watchdog!");
  while (1) {
  }
}

NRF_CLI_CMD_REGISTER(crash, NULL, "trigger a crash", prv_crash_example);
NRF_CLI_CMD_REGISTER(test_log, NULL, "writes test logs to log buffer", prv_test_log);
NRF_CLI_CMD_REGISTER(trigger_logs, NULL, "trigger capture of current log buffer contents", prv_trigger_logs);
NRF_CLI_CMD_REGISTER(trace, NULL, "capture trace event", prv_trace_example);
NRF_CLI_CMD_REGISTER(core_storage_test, NULL, "verify coredump storage implementation with test patterns",
                     prv_coredump_storage_test_cmd);
NRF_CLI_CMD_REGISTER(clear_core, NULL, "clear the core", prv_clear_core_cmd);
NRF_CLI_CMD_REGISTER(get_core, NULL, "gets the core", prv_get_core_cmd);
NRF_CLI_CMD_REGISTER(get_device_info, NULL, "display device information", prv_get_device_info);
NRF_CLI_CMD_REGISTER(reboot, NULL, "reboots system and tracks it with a trace event", prv_system_reboot_cmd);
NRF_CLI_CMD_REGISTER(export, NULL, "Can be used to dump chunks to console or post via GDB", prv_export_data_cmd);
NRF_CLI_CMD_REGISTER(hang, NULL, "Force a hang to test software watchdog functionality", prv_hang_example);

// nrf_cli_help_print() doesn't work from the top level CLI so add a little shim 'help' function
// for better discoverability of memfault added commands
static const nrf_cli_static_entry_t *s_avail_mflt_cmds[] = {
  &CONCAT_3(nrf_cli_, crash, _raw),
  &CONCAT_3(nrf_cli_, test_log, _raw),
  &CONCAT_3(nrf_cli_, trigger_logs, _raw),
  &CONCAT_3(nrf_cli_, trace, _raw),
  &CONCAT_3(nrf_cli_, core_storage_test, _raw),
  &CONCAT_3(nrf_cli_, clear_core, _raw),
  &CONCAT_3(nrf_cli_, get_core, _raw),
  &CONCAT_3(nrf_cli_, get_device_info, _raw),
  &CONCAT_3(nrf_cli_, reboot, _raw),
  &CONCAT_3(nrf_cli_, export, _raw),
  &CONCAT_3(nrf_cli_, hang, _raw),
};

static void prv_help_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv) {
  MEMFAULT_LOG_RAW("Available Memfault Commands:");
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_avail_mflt_cmds); i++) {
    const nrf_cli_static_entry_t *cmd = s_avail_mflt_cmds[i];
    MEMFAULT_LOG_RAW("%s: %s", cmd->p_syntax, cmd->p_help);
  }
}
NRF_CLI_CMD_REGISTER(help, NULL, "Display available memfault commands", prv_help_cmd);
