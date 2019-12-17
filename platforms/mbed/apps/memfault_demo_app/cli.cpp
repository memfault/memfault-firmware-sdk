//! @file
//! @brief
//! This demonstration CLI provides a way to interact with the Mbed
//! platform reference implementation by providing commands to crash
//! and get/delete/print the stored coredump.

#include "cli.h"

#include "mbed.h"
#include "mbed-client-cli/ns_cmdline.h"
#include "memfault/core/debug_log.h"
#include "memfault/demo/cli.h"

static int prv_mflt_retval_to_cmdline_retval(int retval) {
  // https://github.com/ARMmbed/mbed-client-cli/blob/v0.4.0/mbed-client-cli/ns_cmdline.h#L70-L77
  // mbed-client-cli reserves retval codes -5 through 2 for itself
  // and they change the behavior of the cli (e.g. CMDLINE_RETCODE_BUSY)
  // so we can't pass memfault retval codes straight through.
  // the retval is not displayed to the user during the demo.
  return retval == 0 ? CMDLINE_RETCODE_SUCCESS : CMDLINE_RETCODE_FAIL;
}

static int prv_cmd_crash(int argc, char *argv[]) {
  const int retval = memfault_demo_cli_cmd_crash(argc, argv);
  return prv_mflt_retval_to_cmdline_retval(retval);
}

static int prv_cmd_clear_core(int argc, char *argv[]) {
  const int retval = memfault_demo_cli_cmd_clear_core(argc, argv);
  return prv_mflt_retval_to_cmdline_retval(retval);
}

static int prv_cmd_get_core(int argc, char *argv[]) {
  const int retval = memfault_demo_cli_cmd_get_core(argc, argv);
  return prv_mflt_retval_to_cmdline_retval(retval);
}

static int prv_cmd_get_device_info(int argc, char *argv[]) {
  const int retval = memfault_demo_cli_cmd_get_device_info(argc, argv);
  return prv_mflt_retval_to_cmdline_retval(retval);
}

static int prv_cmd_print_chunk(int argc, char *argv[]) {
  const int retval = memfault_demo_cli_cmd_print_chunk(argc, argv);
  return prv_mflt_retval_to_cmdline_retval(retval);
}

void cli_init(void) {
  cmd_init(0);

  // add our demo commands
  cmd_add("crash",            prv_cmd_crash,            "trigger a crash", "");
  cmd_add("clear_core",       prv_cmd_clear_core,       "clear the core", "");
  cmd_add("get_core",         prv_cmd_get_core,         "gets the core", "");
  cmd_add("get_device_info",  prv_cmd_get_device_info,  "display device information", "");
  cmd_add("print_chunk",      prv_cmd_print_chunk,      "Get next Memfault data chunk to send and print as a curl command", "");

  // remove mbed-client-cli built-ins that make the demo more confusing
  cmd_delete("alias");
  cmd_delete("clear");
  cmd_delete("echo");
  cmd_delete("history");
  cmd_delete("set");
  cmd_delete("unset");
  cmd_delete("true");
  cmd_delete("false");
}

MEMFAULT_NORETURN void cli_forever(void) {
  while(true) {
    int c = getchar();
    if (c != EOF) {
        cmd_char_input(c);
    }
  }

  MEMFAULT_UNREACHABLE;
}
