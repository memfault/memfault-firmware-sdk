//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Memfault software watchdog

#include "memfault/config.h"

#if MEMFAULT_DEMO_CLI_WATCHDOG

  #include <stdlib.h>

  #include "memfault/core/compiler.h"
  #include "memfault/core/debug_log.h"
  #include "memfault/ports/watchdog.h"

int memfault_demo_cli_cmd_software_watchdog_enable(MEMFAULT_UNUSED int argc,
                                                   MEMFAULT_UNUSED char *argv[]) {
  int rv = memfault_software_watchdog_enable();
  if (rv < 0) {
    MEMFAULT_LOG_RAW("Failed to enable software watchdog: %d", rv);
    return -1;
  }
  return 0;
}

int memfault_demo_cli_cmd_software_watchdog_disable(MEMFAULT_UNUSED int argc,
                                                    MEMFAULT_UNUSED char *argv[]) {
  int rv = memfault_software_watchdog_disable();
  if (rv < 0) {
    MEMFAULT_LOG_RAW("Failed to disable software watchdog: %d", rv);
    return -1;
  }
  return 0;
}

int memfault_demo_cli_cmd_software_watchdog_update_timeout(MEMFAULT_UNUSED int argc,
                                                           MEMFAULT_UNUSED char *argv[]) {
  if (argc < 2) {
    MEMFAULT_LOG_RAW("Usage: wdog_update <timeout_ms>");
    return -1;
  }

  int timeout_ms = atoi(argv[1]);
  if (timeout_ms <= 0) {
    MEMFAULT_LOG_RAW("Invalid timeout value: %d", timeout_ms);
    return -1;
  }

  int rv = memfault_software_watchdog_update_timeout(timeout_ms);
  if (rv < 0) {
    MEMFAULT_LOG_RAW("Failed to update software watchdog timeout: %d", rv);
    return -1;
  }
  MEMFAULT_LOG_RAW("Software watchdog timeout updated to %d ms", timeout_ms);
  return 0;
}

#endif  // MEMFAULT_DEMO_CLI_WATCHDOG
