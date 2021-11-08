//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Integrate the Memfault demo shell with the mynewt console

#include "memfault_shell.h"

#include <errno.h>
#include <string.h>

#include "console/console.h"
#include "hal/hal_gpio.h"
#include "memfault/components.h"
#include "os/mynewt.h"

#if MYNEWT_VAL(MEMFAULT_CLI)
#include "parse/parse.h"
#include "shell/shell.h"

#if MYNEWT_VAL(BSP_NRF52) || MYNEWT_VAL(BSP_NRF52840)
#include <mcu/nrf52_hal.h>
#elif MYNEWT_VAL(BSP_NRF51)
#include <mcu/nrf51_hal.h>
#elif MYNEWT_VAL(BSP_NRF5340)
#include <mcu/nrf5340_hal.h>
#elif MYNEWT_VAL(BSP_NRF5340_NET)
#include <mcu/nrf5340_net_hal.h>
#elif MYNEWT_VAL(BSP_APOLLO2)
#include <mcu/SYSTEM_APOLLO2.h>
#else
#error "Unsupported arch"
#endif

static int mflt_shell_cmd(int argc, char **argv);

static struct shell_cmd mflt_shell_cmd_struct = {
  .sc_cmd = "mflt",
  .sc_cmd_func = mflt_shell_cmd,
};

static int mflt_shell_err_unknown_arg(char *cmd_name) {
  console_printf("Error: unknown argument \"%s\"\n", cmd_name);
  return SYS_EINVAL;
}

static int mflt_shell_help(void) {
  console_printf("%s cmd\n", mflt_shell_cmd_struct.sc_cmd);
  console_printf("cmd:\n");
  console_printf("\tlogging - Tests memfault logging api\n");
  console_printf(
    "\tcoredump_storage - Runs a sanity test to confirm coredump port is working as expected\n");
  console_printf("\theartbeat - Triggers an immediate heartbeat capture (instead of waiting for "
                 "timer to expire\n");
  console_printf("\ttrace - Test trace event\n");
  console_printf("\treboot - Trigger a user initiated reboot and confirm reason is persisted\n");
  console_printf("\tassert - Triggers memfault assert where a coredump should be captured\n");
  console_printf("\tfault - Triggers memfault fault where a coredump should be captured \n");
  console_printf("\thang - Triggers memfault hang where a coredump should be captured \n");
  console_printf("\texport - Dump Memfault data collected to console\n");

  return SYS_EOK;
}

//
// Test Platform Ports
//

static int test_logging(int argc, char *argv[]) {
  MEMFAULT_LOG_DEBUG("Debug log!");
  MEMFAULT_LOG_INFO("Info log!");
  MEMFAULT_LOG_WARN("Warning log!");
  MEMFAULT_LOG_ERROR("Error log!");
  return 0;
}

// Runs a sanity test to confirm coredump port is working as expected
static int test_coredump_storage(int argc, char *argv[]) {
  // Note: Coredump saving runs from an ISR prior to reboot so should
  // be safe to call with interrupts disabled.
  int sr;
  __HAL_DISABLE_INTERRUPTS(sr);
  memfault_coredump_storage_debug_test_begin();
  __HAL_ENABLE_INTERRUPTS(sr);

  memfault_coredump_storage_debug_test_finish();
  return 0;
}

//
// Test core SDK functionality
//

// Triggers an immediate heartbeat capture (instead of waiting for timer
// to expire)
static int test_heartbeat(int argc, char *argv[]) {
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
}

static int test_trace(int argc, char *argv[]) {
  MEMFAULT_TRACE_EVENT_WITH_LOG(Unknown, "A test error trace!");
  return 0;
}

//! Trigger a user initiated reboot and confirm reason is persisted
static int test_reboot(int argc, char *argv[]) {
  // memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
}

//
// Test different crash types where a coredump should be captured
//

static int test_assert(int argc, char *argv[]) {
  MEMFAULT_ASSERT(0);
  return -1;  // should never get here
}

static int test_fault(int argc, char *argv[]) {
  void (*bad_func)(void) = (void *)0xEEEEDEAD;
  bad_func();
  return -1;  // should never get here
}

static int test_hang(int argc, char *argv[]) {
  while (1) {
  }
  return -1;  // should never get here
}

// Dump Memfault data collected to console
static int test_export(int argc, char *argv[]) {
  memfault_data_export_dump_chunks();
  return 0;
}

static int mflt_shell_cmd(int argc, char **argv) {
  if (argc < 2) {
    return mflt_shell_help();
  }

  struct subcommand {
    const char *name;
    int (*func)(int argc, char **argv);
  };
  static const struct subcommand subcommands[] = {
    { "logging", test_logging },
    { "coredump_storage", test_coredump_storage },
    { "heartbeat", test_heartbeat },
    { "trace", test_trace },
    { "reboot", test_reboot },
    { "assert", test_assert },
    { "fault", test_fault },
    { "hang", test_hang },
    { "export", test_export },
  };

  for (int i = 0; i < sizeof(subcommands) / sizeof(subcommands[0]); i++) {
    if (strcmp(argv[1], subcommands[i].name) == 0) {
      return subcommands[i].func(argc - 1, argv + 1);
    }
  }

  return mflt_shell_err_unknown_arg(argv[1]);
}

int mflt_shell_init(void) {
  int rc = shell_cmd_register(&mflt_shell_cmd_struct);
  SYSINIT_PANIC_ASSERT(rc == 0);

  return rc;
}
#endif
