//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include "memfault/core/reboot_tracking.h"

void memfault_reboot_tracking_mark_reset_imminent(
  MEMFAULT_UNUSED eMemfaultRebootReason reboot_reason,
  MEMFAULT_UNUSED const sMfltRebootTrackingRegInfo *reg) {
  return;
}

int memfault_reboot_tracking_get_reboot_reason(MEMFAULT_UNUSED sMfltRebootReason *reboot_reason) {
  return 0;
}
