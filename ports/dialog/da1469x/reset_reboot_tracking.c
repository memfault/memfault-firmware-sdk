//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

//! @file
//!
//! Map DA1469x reboot reasons to memfault definitions

#include "DA1469xAB.h"
#include "memfault/ports/reboot_reason.h"
#include "sdk_defs.h"

__RETAINED_UNINIT static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  eMemfaultRebootReason reset_reason;

  uint32_t reset_cause = CRG_TOP->RESET_STAT_REG;

  // Multiple reset bits may be set in different cases. The order these are checked is important
  // Checks should be done from POR -> HW -> SW, and from most to least specific
  // First, check if reset bits are 0 as this indicates return from deepsleep
  // Next, check POR bit. POR bit set for power issue (brown-out, vdd drop, batt drop), or POR event
  // NB: If POR occurs, all bits are set
  // Next, check for SWD_HWRESET bit, special type of hardware reset
  // Next, check for one of watchdog bits (CMAC or WDOG)
  // Next, check for HW_RESET, indicates reset pin was asserted
  // Finally, check for SW_RESET
  // See exception_handlers.S:Wakeup_Reset_Handler in DA1469x SDK, Figure 26 in datasheet
  if (reset_cause == 0) {
    reset_reason = kMfltRebootReason_DeepSleep;
  } else if (reset_cause & CRG_TOP_RESET_STAT_REG_PORESET_STAT_Msk) {
    reset_reason = kMfltRebootReason_PowerOnReset;
  } else if (reset_cause & CRG_TOP_RESET_STAT_REG_SWD_HWRESET_STAT_Msk) {
    reset_reason = kMfltRebootReason_DebuggerHalted;
  } else if (reset_cause & (CRG_TOP_RESET_STAT_REG_CMAC_WDOGRESET_STAT_Msk |
                            CRG_TOP_RESET_STAT_REG_WDOGRESET_STAT_Msk)) {
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & CRG_TOP_RESET_STAT_REG_HWRESET_STAT_Msk) {
    reset_reason = kMfltRebootReason_PinReset;
  } else if (reset_cause & CRG_TOP_RESET_STAT_REG_SWRESET_STAT_Msk) {
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else {
    reset_reason = kMfltRebootReason_Unknown;
  }

#if MEMFAULT_REBOOT_REASON_CLEAR
  CRG_TOP->RESET_STAT_REG = 0;
#endif

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
