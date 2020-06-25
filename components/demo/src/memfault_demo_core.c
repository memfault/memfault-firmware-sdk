//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Memfault SDK

#include "memfault/demo/cli.h"

#include <stdlib.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/reboot_tracking.h"

int memfault_demo_cli_cmd_get_device_info(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  struct MemfaultDeviceInfo info = {0};
  memfault_platform_get_device_info(&info);
  MEMFAULT_LOG_INFO("S/N: %s", info.device_serial ? info.device_serial : "<NULL>");
  MEMFAULT_LOG_INFO("SW type: %s", info.software_type ? info.software_type : "<NULL>");
  MEMFAULT_LOG_INFO("SW version: %s", info.software_version ? info.software_version : "<NULL>");
  MEMFAULT_LOG_INFO("HW version: %s", info.hardware_version ? info.hardware_version : "<NULL>");
  return 0;
}

int memfault_demo_cli_cmd_system_reboot(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  void *pc;
  MEMFAULT_GET_PC(pc);
  void *lr;
  MEMFAULT_GET_LR(lr);
  sMfltRebootTrackingRegInfo reg_info = {
    .pc = (uint32_t)(uintptr_t)pc,
    .lr = (uint32_t)(uintptr_t)lr,
  };

  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, &reg_info);
  memfault_platform_reboot();
  return 0;
}
