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
#include "stm32f769xx.h"
#include "stm32f7xx_hal.h"

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
#define MEMFAULT_PRINT_RESET_INFO(...)
#endif

//! Mappings come from "5.3.21 RCC clock control & status register (RCC_CSR)" of
//! the ST "RM0410" Reference Manual for (STM32F76xxx and STM32F77xxx).
typedef enum ResetSource {
  kResetSource_PwrPor = (RCC_CSR_PORRSTF_Msk),
  kResetSource_Pin = (RCC_CSR_PINRSTF_Msk),
  kResetSource_PwrBor = (RCC_CSR_BORRSTF_Msk),
  kResetSource_Software = (RCC_CSR_SFTRSTF_Msk),
  kResetSource_Wwdg = (RCC_CSR_WWDGRSTF_Msk),
  kResetSource_Iwdg = (RCC_CSR_IWDGRSTF_Msk),
  kResetSource_LowPwr = (RCC_CSR_LPWRRSTF_Msk),
} eResetSource;

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RCC->CSR;

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_PRINT_RESET_INFO("Reset Reason, RCC_CSR=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  const uint32_t reset_mask_all =
    (RCC_CSR_BORRSTF_Msk | RCC_CSR_PINRSTF_Msk | RCC_CSR_PORRSTF_Msk | RCC_CSR_SFTRSTF_Msk |
     RCC_CSR_IWDGRSTF_Msk | RCC_CSR_WWDGRSTF_Msk | RCC_CSR_LPWRRSTF_Msk);

  switch (reset_cause & reset_mask_all) {
    case kResetSource_PwrPor:
      MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
      reset_reason = kMfltRebootReason_PowerOnReset;
      break;
    case kResetSource_Pin:
      MEMFAULT_PRINT_RESET_INFO(" Pin Reset");
      reset_reason = kMfltRebootReason_PinReset;
      break;
    case kResetSource_PwrBor:
      MEMFAULT_PRINT_RESET_INFO(" Brown out");
      reset_reason = kMfltRebootReason_BrownOutReset;
      break;
    case kResetSource_Software:
      MEMFAULT_PRINT_RESET_INFO(" Software");
      reset_reason = kMfltRebootReason_SoftwareReset;
      break;
    case kResetSource_Wwdg:
      MEMFAULT_PRINT_RESET_INFO(" Window Watchdog");
      reset_reason = kMfltRebootReason_HardwareWatchdog;
      break;
    case kResetSource_Iwdg:
      MEMFAULT_PRINT_RESET_INFO(" Independent Watchdog");
      reset_reason = kMfltRebootReason_HardwareWatchdog;
      break;
    case kResetSource_LowPwr:
      MEMFAULT_PRINT_RESET_INFO(" Low power management");
      reset_reason = kMfltRebootReason_LowPower;
      break;
    default:
      MEMFAULT_PRINT_RESET_INFO(" Unknown");
      break;
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
