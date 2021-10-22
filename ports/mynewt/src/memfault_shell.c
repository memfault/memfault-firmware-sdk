#include "memfault_shell.h"
#include "memfault/components.h"
#include <string.h>
#include <errno.h>
#include "os/mynewt.h"
#include "console/console.h"
#include "hal/hal_gpio.h"

#if MYNEWT_VAL(MEMFAULT_CLI)
#include "shell/shell.h"
#include "parse/parse.h"

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
  .sc_cmd_func = mflt_shell_cmd
};

static int
mflt_shell_err_unknown_arg(char *cmd_name)
{
  console_printf("Error: unknown argument \"%s\"\n",
      cmd_name);
  return SYS_EINVAL;
}

static int
mflt_shell_help(void)
{
  console_printf("%s cmd\n", mflt_shell_cmd_struct.sc_cmd);
  console_printf("cmd:\n");
  console_printf("\tlogging // Tests memfault logging api\n");
  console_printf("\tcoredump_storage // Runs a sanity test to confirm coredump port is working as expected\n");
  console_printf("\theartbeat // Triggers an immediate heartbeat capture (instead of waiting for timer to expire\n");
  console_printf("\ttrace // Test trace event\n");
  console_printf("\treboot // Trigger a user initiated reboot and confirm reason is persisted\n");
  console_printf("\tassert // Triggers memfault assert where a coredump should be captured\n");
  console_printf("\tfault // Triggers memfault fault where a coredump should be captured \n");
  console_printf("\thang // Triggers memfault hang where a coredump should be captured \n");
  console_printf("\texport // Dump Memfault data collected to console\n");

  return SYS_EOK;
}

//
// Test Platform Ports
//

static int
test_logging(int argc, char *argv[])
{
  MEMFAULT_LOG_DEBUG("Debug log!");
  MEMFAULT_LOG_INFO("Info log!");
  MEMFAULT_LOG_WARN("Warning log!");
  MEMFAULT_LOG_ERROR("Error log!");
  return 0;
}

// Runs a sanity test to confirm coredump port is working as expected
static int
test_coredump_storage(int argc, char *argv[])
{

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
static int
test_heartbeat(int argc, char *argv[])
{
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
}

static int
test_trace(int argc, char *argv[])
{
  MEMFAULT_TRACE_EVENT_WITH_LOG(Unknown, "A test error trace!");
  return 0;
}

//! Trigger a user initiated reboot and confirm reason is persisted
static int
test_reboot(int argc, char *argv[])
{
  // memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, NULL);
  memfault_platform_reboot();
}

//
// Test different crash types where a coredump should be captured
//

static int
test_assert(int argc, char *argv[])
{
  MEMFAULT_ASSERT(0);
  return -1; // should never get here
}

static int
test_fault(void)
{
  void (*bad_func)(void) = (void *)0xEEEEDEAD;
  bad_func();
  return -1; // should never get here
}

static int
test_hang(int argc, char *argv[])
{
  while (1) {}
  return -1; // should never get here
}

// Dump Memfault data collected to console
static int
test_export(int argc, char *argv[])
{
  memfault_data_export_dump_chunks();
  return 0;
}

static int
mflt_shell_cmd(int argc, char **argv)
{
  if (argc == 1) {
    return mflt_shell_help();
  }

  if (argc > 1 && strcmp(argv[1], "logging") == 0) {
    return test_logging(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "coredump_storage") == 0) {
    return test_coredump_storage(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "heartbeat") == 0) {
    return test_heartbeat(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "trace") == 0) {
    return test_trace(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "reboot") == 0) {
    return test_reboot(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "assert") == 0) {
    return test_assert(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "fault") == 0) {
    return test_fault();
  }

  if (argc > 1 && strcmp(argv[1], "hang") == 0) {
    return test_hang(argc, argv);
  }

  if (argc > 1 && strcmp(argv[1], "export") == 0) {
    return test_export(argc, argv);
  }

  return mflt_shell_err_unknown_arg(argv[1]);
}

int
mflt_shell_init(void)
{
  int rc;

  rc = shell_cmd_register(&mflt_shell_cmd_struct);
  SYSINIT_PANIC_ASSERT(rc == 0);

  return rc;
}
#endif