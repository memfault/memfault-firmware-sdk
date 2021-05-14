//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A minimal example app for exercising and trying out the Memfault SDK with the DA145xx SDK.

#include "app_api.h"
#include "user_app.h"

#ifdef CFG_PRINTF
#include "uart.h"
#include "arch_console.h"
#endif

#include "memfault/components.h"

static int prv_send_char(char c) {
  arch_printf("%c", c);
  return 0;
}

//! A custom callback invoked by the Dialog SDK on bootup. See user_callback_config.h for
//! configuration.
void user_app_on_init(void) {
  memfault_platform_boot();

  //! We will use the Memfault CLI shell as a very basic debug interface
  const sMemfaultShellImpl impl = {
    .send_char = prv_send_char,
  };
  memfault_demo_shell_boot(&impl);

  default_app_on_init();
}

void user_on_connection(uint8_t connection_idx,
                        struct gapc_connection_req_ind const *param) {
  default_app_on_connection(connection_idx, param);
}

void user_on_disconnect(struct gapc_disconnect_ind const *param) {
  default_app_on_disconnect(param);
}

#if !defined (__DA14531__)
static int prv_test_storage(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  GLOBAL_INT_DISABLE();
  memfault_coredump_storage_debug_test_begin();
  GLOBAL_INT_RESTORE();
  memfault_coredump_storage_debug_test_finish();
  return 0;
}
#endif

static int prv_export_data(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  memfault_data_export_dump_chunks();
  return 0;
}

static const sMemfaultShellCommand s_memfault_shell_commands[] = {
  {"crash", memfault_demo_cli_cmd_crash, ""},
#if !defined (__DA14531__) // Don't use on DA14531 to reduce memory usage
  {"trace", memfault_demo_cli_cmd_trace_event_capture, ""},
  {"get_core", memfault_demo_cli_cmd_get_core, ""},
  {"clear_core", memfault_demo_cli_cmd_clear_core, ""},
  {"test_storage", prv_test_storage, ""},
  {"get_device_info", memfault_demo_cli_cmd_get_device_info, ""},
#endif
  {"reboot", memfault_demo_cli_cmd_system_reboot, ""},
  {"export", prv_export_data, ""},
  {"help", memfault_shell_help_handler, ""},
};

const sMemfaultShellCommand *const g_memfault_shell_commands = s_memfault_shell_commands;
const size_t g_memfault_num_shell_commands = MEMFAULT_ARRAY_SIZE(s_memfault_shell_commands);

//! Called by the DA145xx scheduler
arch_main_loop_callback_ret_t user_app_on_system_powered(void) {
  while (uart_data_ready_getf(UART2) > 0) {
    uint8_t rx_byte = uart_read_byte(UART2);
    memfault_demo_shell_receive_char((char)rx_byte);
    arch_printf_process();
  }

  return GOTO_SLEEP;
}
