//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset and Clock Control" (RCC)'s "control & status register" (CSR) Register.
//!
//! More details can be found in the "RCC control/status register (RCC_CSR)"
//! section of the STM32U5 family reference manual, RM0456.

#include "memfault/components.h"
#include "stm32u5xx_ll_rcc.h"

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  #define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
  #define MEMFAULT_PRINT_RESET_INFO(...)
#endif

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  MEMFAULT_SDK_ASSERT(info != NULL);

  const uint32_t reset_cause = RCC->CSR;

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  MEMFAULT_PRINT_RESET_INFO("Reset Reason, RCC_CSR=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  if (LL_RCC_IsActiveFlag_LPWRRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Low Power");
    reset_reason = kMfltRebootReason_DeepSleep;
  } else if (LL_RCC_IsActiveFlag_SFTRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Software");
    reset_reason = kMfltRebootReason_SoftwareReset;
  } else if (LL_RCC_IsActiveFlag_IWDGRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Independent Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  } else if (LL_RCC_IsActiveFlag_WWDGRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Window Watchdog");
    reset_reason = kMfltRebootReason_HardwareWatchdog;
  }
  // Per the reference manual, RM0456 "11.8.50 RCC control/status register
  // (RCC_CSR)", the reset value for this register is 0x0c00_4400, which is both
  // PINRST and BORRST bits set. So, we need to check for this case first.
  else if (LL_RCC_IsActiveFlag_PINRST() && LL_RCC_IsActiveFlag_BORRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kMfltRebootReason_PowerOnReset;
  } else if (LL_RCC_IsActiveFlag_BORRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Brown out");
    reset_reason = kMfltRebootReason_BrownOutReset;
  } else if (LL_RCC_IsActiveFlag_PINRST()) {
    MEMFAULT_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kMfltRebootReason_PinReset;
  } else {
    MEMFAULT_PRINT_RESET_INFO(" Unknown");
  }

  // we have read the reset information so clear the bits (since they are sticky across reboots)
  LL_RCC_ClearResetFlags();

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
