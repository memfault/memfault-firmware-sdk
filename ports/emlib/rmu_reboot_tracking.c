//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset Management Unit" (RMU)'s "Reset Cause" (RSTCAUSE) Register.
//!
//! More details can be found in the "RMU_RSTCAUSE Register" of the reference manual
//! for the specific EFR or EFM chip family. It makes use of APIs that are part of
//! the Gecko SDK.

#include "memfault/ports/reboot_reason.h"

#include "em_rmu.h"

#include "memfault/core/debug_log.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/sdk_assert.h"

#ifndef MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_ENABLE_REBOOT_DIAG_DUMP 1
#endif

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
#define MEMFAULT_PRINT_RESET_INFO(...)
#endif

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  MEMFAULT_SDK_ASSERT(info != NULL);

  // This routine simply reads RMU->RSTCAUSE and zero's out
  // bits that aren't relevant to the reset. For more details
  // check out the logic in ${PATH_TO_GECKO_SDK}/platform/emlib/src/em_rmu.c
  const uint32_t reset_cause = RMU_ResetCauseGet();

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_LOG_INFO("Reset Reason, RSTCAUSE=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  // Find the RMU_RSTCAUSE Register data sheet for the EFM/EFR part for more details
  // For example, in the EFM32PG12 it's in section "8.3.2 RMU_RSTCAUSE Register"
  //
  // Note that some reset types are shared across EFM/EFR MCU familes. For the ones
  // that are not, we wrap the reason with an ifdef.
  if (reset_cause & RMU_RSTCAUSE_PORST) {
    MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kMfltRebootReason_PowerOnReset;
#if defined(RMU_RSTCAUSE_AVDDBOD)
  } else if (reset_cause & RMU_RSTCAUSE_AVDDBOD) {
    MEMFAULT_PRINT_RESET_INFO(" AVDD Brown Out");
    reset_reason = kMfltRebootReason_BrownOutReset;
#endif
#if defined(RMU_RSTCAUSE_DVDDBOD)
  } else if (reset_cause & RMU_RSTCAUSE_DVDDBOD) {
    MEMFAULT_PRINT_RESET_INFO(" DVDD Brown Out");
    reset_reason = kMfltRebootReason_BrownOutReset;
#endif
#if defined(RMU_RSTCAUSE_DECBOD)
  } else if (reset_cause & RMU_RSTCAUSE_DECBOD) {
    MEMFAULT_PRINT_RESET_INFO(" DEC Brown Out");
    reset_reason = kMfltRebootReason_BrownOutReset;
#endif
  } else if (reset_cause & RMU_RSTCAUSE_LOCKUPRST) {
    MEMFAULT_PRINT_RESET_INFO(" Lockup");
    reset_reason = kMfltRebootReason_Lockup;
  } else if (reset_cause & RMU_RSTCAUSE_SYSREQRST) {
    MEMFAULT_PRINT_RESET_INFO(" Software");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (reset_cause & RMU_RSTCAUSE_WDOGRST) {
    MEMFAULT_PRINT_RESET_INFO(" Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
#if defined(RMU_RSTCAUSE_EM4WURST)
  } else if (reset_cause & RMU_RSTCAUSE_EM4RST) {
    MEMFAULT_PRINT_RESET_INFO(" EM4 Wakeup");
    reset_reason = kMfltRebootReason_DeepSleep;
#endif
  } else if (reset_cause & RMU_RSTCAUSE_EXTRST) {
    MEMFAULT_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kMfltRebootReason_ButtonReset;
  } else {
    MEMFAULT_PRINT_RESET_INFO(" Unknown");
  }

  // we have read the reset information so clear the bits (since they are sticky across reboots)
  RMU->CMD = RMU_CMD_RCCLR;

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
