//! Fault handling for Xtensa based devices

#include "memfault/panics/reboot_tracking.h"
#include "memfault_reboot_tracking_private.h"

static eMfltRebootReason s_crash_reason = kMfltRebootReason_Unknown;

void memfault_fault_handling_assert(void *pc, void *lr, uint32_t extra) {
  // TODO
}
