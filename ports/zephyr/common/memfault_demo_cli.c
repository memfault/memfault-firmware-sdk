//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Adds a basic set of commands for interacting with Memfault SDK

#include "memfault/demo/cli.h"

#include <shell/shell.h>

#include "memfault_zephyr_http.h"

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

static int prv_print_core_cmd(const struct shell *shell, size_t argc, char **argv) {
  return memfault_demo_cli_cmd_print_core(argc, argv);
}

static int prv_post_data_cmd(const struct shell *shell, size_t argc, char **argv) {
#if defined(CONFIG_MEMFAULT_HTTP_SUPPORT)
  return memfault_zephyr_port_post_data();
#else
  shell_print(shell, "CONFIG_MEMFAULT_HTTP_SUPPORT not enabled");
  return 0;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_memfault_cmds,
    SHELL_CMD(crash, NULL, "trigger a crash", prv_crash_example),
    SHELL_CMD(clear_core, NULL, "clear the core", prv_clear_core_cmd),
    SHELL_CMD(get_core, NULL, "gets the core", prv_get_core_cmd),
    SHELL_CMD(get_device_info, NULL, "display device information", prv_get_device_info),
    SHELL_CMD(print_msg, NULL, "get next Memfault data to send and print as a curl command",
              prv_print_core_cmd),
    SHELL_CMD(post_msg, NULL, "get next Memfault data to send and POST it to the Memfault Cloud",
              prv_post_data_cmd),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mflt, &sub_memfault_cmds, "Memfault Test Commands", NULL);
