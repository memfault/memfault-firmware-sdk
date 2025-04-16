//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Extract system reboot reason from the PMU.SYS_RST_STATUS register.

#include <inttypes.h>

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#if defined(RW610)
  #include "RW610.h"
#else
  // Default to the RW612 header file. If a user needs the RW610 header file,
  // they can define RW610 in their project settings or in
  // memfault_platform_config.h
  #include "RW612.h"
#endif

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  #define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
  #define MEMFAULT_PRINT_RESET_INFO(...)
#endif

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  // Undocumented: on reset, the ROM bootloader backs up PMU->SYS_RST_STATUS to
  // RF_SYSCON->WO_SCRATCH_REG[3] and clears the PMU->SYS_RST_STATUS register.
  // https://github.com/zephyrproject-rtos/hal_nxp/blob/8c354a918c1272b40ad9b4ffecac1d89125efbe6/mcux/mcux-sdk/devices/RW610/drivers/fsl_power.h#L259-L264
  const uint32_t reset_cause = RF_SYSCON->WO_SCRATCH_REG[3];

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  // Clear the SRC Reset Status Register by writing a 1 to each bit. This is
  // done automatically by the ROM bootloader on reset already, but it doesn't
  // hurt to do it again.
#if MEMFAULT_REBOOT_REASON_CLEAR
  SRC->SYS_RST_CLR = reset_cause;
#endif

  MEMFAULT_LOG_INFO("Reset Reason, SYS_RST_STATUS=0x%" PRIx32, reset_cause);

  MEMFAULT_PRINT_RESET_INFO("Reset Cause: ");
  if (reset_cause & PMU_SYS_RST_STATUS_CM33_SYSRESETREQ_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" Software");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (reset_cause & PMU_SYS_RST_STATUS_CM33_LOCKUP_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" Lockup");
    reset_reason = kMfltRebootReason_Lockup;
  } else if (reset_cause & PMU_SYS_RST_STATUS_WDT_RST_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" HW Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & PMU_SYS_RST_STATUS_AP_SYSRESETREQ_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" Debugger");
    reset_reason = kMfltRebootReason_DebuggerHalted;
  } else if (reset_cause & PMU_SYS_RST_STATUS_CODE_WDT_RST_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" SW Watchdog");
    reset_reason = kMfltRebootReason_SoftwareWatchdog;
  } else if (reset_cause & PMU_SYS_RST_STATUS_ITRC_CHIP_RST_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" Tamper");
    reset_reason = kMfltRebootReason_SecurityViolation;
  } else if (reset_cause & PMU_SYS_RST_STATUS_SW_RESETB_SCANTEST_MASK) {
    MEMFAULT_PRINT_RESET_INFO(" Resetb");
    // Unfortunately this doesn't seem to be set on pin reset
    reset_reason = kMfltRebootReason_PinReset;
  } else {
    // Power On Reset and Pin Reset are unfortunately not distinguished in the
    // reset cause register. Default to Power On Reset.
    MEMFAULT_PRINT_RESET_INFO(" POR");
    reset_reason = kMfltRebootReason_PowerOnReset;
  }

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
