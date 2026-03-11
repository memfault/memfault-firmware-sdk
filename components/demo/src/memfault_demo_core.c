//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Memfault SDK

#include <stdlib.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/device_info.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/demo/cli.h"

int memfault_demo_cli_cmd_get_device_info(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  memfault_device_info_dump();
  return 0;
}

int memfault_demo_cli_cmd_system_reboot(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  MEMFAULT_REBOOT_MARK_RESET_IMMINENT(kMfltRebootReason_UserReset);
  memfault_platform_reboot();
  return 0;
}
