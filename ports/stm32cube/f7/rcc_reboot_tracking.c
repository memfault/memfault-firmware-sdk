//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the "Reset and
//! Clock Control" (RCC)'s "Control & Status Register" (CSR).
//!
//! More details can be found in the "RCC clock control & status register
//! (RCC_CSR)" section of the STM32F7 family reference manual.

#include "memfault/components.h"
#include "stm32f7xx_hal.h"

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
#define MEMFAULT_PRINT_RESET_INFO(...)
#endif

//! Reset reason codes come from "5.3.21 RCC clock control & status register
//! (RCC_CSR)" of the ST "RM0410" Reference Manual for (STM32F76xxx and
//! STM32F77xxx).
void memfault_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RCC->CSR;

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_PRINT_RESET_INFO("Reset Reason, RCC_CSR=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  // look for the first bit set in the reset cause register.
  //
  // pin reset is checked last; all other internally generated resets are wired
  // to the reset pin, see section 5.1.2 of the Reference Manual.
  if (reset_cause & RCC_CSR_SFTRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Software");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (reset_cause & RCC_CSR_PORRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kMfltRebootReason_PowerOnReset;
  } else if (reset_cause & RCC_CSR_BORRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Brown out");
    reset_reason = kMfltRebootReason_BrownOutReset;
  } else if (reset_cause & RCC_CSR_WWDGRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Window Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_CSR_IWDGRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Independent Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_CSR_LPWRRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Low Power");
    reset_reason = kMfltRebootReason_LowPower;
  } else if (reset_cause & RCC_CSR_PINRSTF_Msk) {
    MEMFAULT_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kMfltRebootReason_PinReset;
  } else {
    MEMFAULT_PRINT_RESET_INFO(" Unknown");
    reset_reason = kMfltRebootReason_Unknown;
  }

#if MEMFAULT_REBOOT_REASON_CLEAR
  // we have read the reset information so clear the bits (since they are sticky
  // across reboots)
  __HAL_RCC_CLEAR_RESET_FLAGS();
#endif

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
