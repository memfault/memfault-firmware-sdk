//! @file memfault_platform_reboot_tracking.c
//!
//! Copyright 2022 Memfault, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! Glue layer between the Memfault SDK and the Nuttx platform

/*****************************************************************************
 * Included Files
 *****************************************************************************/

#include "memfault/ports/reboot_reason.h"

#include <sys/boardctl.h>

#include "memfault/config.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/reboot_reason_types.h"
#include "memfault/core/sdk_assert.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
#define MEMFAULT_PRINT_RESET_INFO(...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

MEMFAULT_PUT_IN_SECTION(".noinit.mflt_reboot_tracking")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memfault_reboot_reason_get
 ****************************************************************************/

/**
 * @brief Transform Nuttx provided reset reasons into memfault 
 * compatible ones.
 * 
 * @param info 
 */

void memfault_reboot_reason_get(sResetBootupInfo *info) {

  uint32_t reset_reason_reg = 0x0;
  eMemfaultRebootReason reset_reason = kMfltRebootReason_UnknownError;

#ifdef CONFIG_BOARDCTL_RESET_CAUSE

  struct boardioc_reset_cause_s cause;
  if (boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&cause) != OK)
    {
      syslog(LOG_ERR, "Failed to get reset cause!\n");
    }

  reset_reason_reg = (uint32_t)cause.cause;

  MEMFAULT_LOG_INFO("Reset Reason, GetResetReason=0x%" PRIx32, reset_reason_reg);
  MEMFAULT_PRINT_RESET_INFO("Reset Causes: ");

  switch (reset_reason_reg) {
    case BOARDIOC_RESETCAUSE_SYS_CHIPPOR:
      MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
      reset_reason = kMfltRebootReason_PowerOnReset;
      break;
    case BOARDIOC_RESETCAUSE_PIN:
      MEMFAULT_PRINT_RESET_INFO(" User Reset");
      reset_reason = kMfltRebootReason_UserReset;
      break;
    case BOARDIOC_RESETCAUSE_SYS_BOR:
      MEMFAULT_PRINT_RESET_INFO(" Brown out");
      reset_reason = kMfltRebootReason_BrownOutReset;
      break;
    case BOARDIOC_RESETCAUSE_CPU_SOFT:
    case BOARDIOC_RESETCAUSE_CORE_SOFT:
      MEMFAULT_PRINT_RESET_INFO(" Software");
      reset_reason = kMfltRebootReason_SoftwareReset;
      break;
    case BOARDIOC_RESETCAUSE_SYS_RWDT:
    case BOARDIOC_RESETCAUSE_CORE_MWDT:
    case BOARDIOC_RESETCAUSE_CPU_MWDT:
    case BOARDIOC_RESETCAUSE_CORE_RWDT:
    case BOARDIOC_RESETCAUSE_CPU_RWDT:
      MEMFAULT_PRINT_RESET_INFO(" Watchdog");
      reset_reason = kMfltRebootReason_HardwareWatchdog;
      break;
    case BOARDIOC_RESETCAUSE_LOWPOWER:
      MEMFAULT_PRINT_RESET_INFO(" Low Power Exit");
      reset_reason = kMfltRebootReason_LowPower;
      break;
    case BOARDIOC_RESETCAUSE_CORE_DPSP:
      MEMFAULT_PRINT_RESET_INFO(" Deep Sleep");
      reset_reason = kMfltRebootReason_DeepSleep;
      break;
    case BOARDIOC_RESETCAUSE_NONE:
    case BOARDIOC_RESETCAUSE_UNKOWN:
    default:
      MEMFAULT_PRINT_RESET_INFO(" Unknown");
      reset_reason = kMfltRebootReason_Unknown;
      break;
  }

#endif

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_reason_reg,
    .reset_reason = reset_reason,
  };
}

/****************************************************************************
 * Name: memfault_platform_reboot_tracking_boot
 ****************************************************************************/

/**
 * @brief Tracking of reoboot causes.
 * 
 */

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}