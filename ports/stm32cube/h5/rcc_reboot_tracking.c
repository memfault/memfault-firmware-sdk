//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset and Clock Control" (RCC)'s "Reset Status Register" (RSR).
//!
//! More details can be found in the "RCC Reset Status Register (RCC_RSR)"
//! section of the STM32H5 family reference manual.

#include "memfault/config.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/ports/reboot_reason.h"
#include "stm32h5xx_ll_rcc.h"

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  #define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
  #define MEMFAULT_PRINT_RESET_INFO(...)
#endif

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RCC->RSR;

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_PRINT_RESET_INFO("Reset Reason, RCC_RSR=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  if (reset_cause & RCC_RSR_PINRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kMfltRebootReason_PinReset;
  } else if (reset_cause & RCC_RSR_BORRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Brown out");
    reset_reason = kMfltRebootReason_BrownOutReset;
  } else if (reset_cause & RCC_RSR_SFTRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Software");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (reset_cause & RCC_RSR_IWDGRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Independent Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_RSR_WWDGRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Window Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_RSR_LPWRRSTF) {
    MEMFAULT_PRINT_RESET_INFO(" Low Power");
    reset_reason = kMfltRebootReason_LowPower;
  }

#if MEMFAULT_REBOOT_REASON_CLEAR
  // we have read the reset information so clear the bits (since they are sticky across reboots)
  __HAL_RCC_CLEAR_RESET_FLAGS();
#endif

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
