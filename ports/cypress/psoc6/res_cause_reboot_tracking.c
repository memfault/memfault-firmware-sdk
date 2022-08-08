//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the Reset
//! Reason using the PSOC6 Peripheral HAL: https://github.com/Infineon/mtb-pdl-cat1

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#include "cy_syslib.h"

//! Note: The default PSOC62 linker scripts have a KEEP(*(.noinit)) that we can use
MEMFAULT_PUT_IN_SECTION(".mflt_reboot_tracking.noinit")
static uint8_t s_reboot_tracking[MEMFAULT_REBOOT_TRACKING_REGION_SIZE];

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
#define MEMFAULT_PRINT_RESET_INFO(...)
#endif

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  MEMFAULT_SDK_ASSERT(info != NULL);

  eMemfaultRebootReason reset_reason = kMfltRebootReason_Unknown;

  const uint32_t reset_cause = Cy_SysLib_GetResetReason();

  MEMFAULT_LOG_INFO("Reset Reason, Cy_SysLib_GetResetReason=0x%" PRIx32, reset_cause);
  MEMFAULT_PRINT_RESET_INFO("Reset Cause: ");

  switch (reset_cause) {
    case 0:
      // Per code comments indicates "POR, XRES, or BOD":
      //  https://github.com/Infineon/mtb-pdl-cat1/blob/3c6aebd/personalities/platform/power-1.3.cypersonality#L205
      MEMFAULT_PRINT_RESET_INFO(" POR, XRES, or BOD");
      reset_reason = kMfltRebootReason_PowerOnReset;
      break;

    case CY_SYSLIB_RESET_HWWDT:
      MEMFAULT_PRINT_RESET_INFO(" HW WDT");
      reset_reason = kMfltRebootReason_HardwareWatchdog;
      break;
    case CY_SYSLIB_RESET_ACT_FAULT:
      // unauthorized protection violations
      MEMFAULT_PRINT_RESET_INFO(" ACT Fault");
      reset_reason = kMfltRebootReason_UnknownError;
      break;
    case CY_SYSLIB_RESET_DPSLP_FAULT:
      // failed to enter deep sleep
      MEMFAULT_PRINT_RESET_INFO(" DPSLP Fault");
      reset_reason = kMfltRebootReason_UnknownError;
      break;

#if defined (CY_IP_M33SYSCPUSS) || defined (CY_IP_M7CPUSS)
    case CY_SYSLIB_RESET_TC_DBGRESET:
      MEMFAULT_PRINT_RESET_INFO(" TC DBGRESET");
      reset_reason = kMfltRebootReason_Assert;
      break;
#endif

    case CY_SYSLIB_RESET_SOFT:
      MEMFAULT_PRINT_RESET_INFO(" Software Reset");
      reset_reason = kMfltRebootReason_SoftwareReset;
      break;

    case CY_SYSLIB_RESET_SWWDT0:
    case CY_SYSLIB_RESET_SWWDT1:
    case CY_SYSLIB_RESET_SWWDT2:
    case CY_SYSLIB_RESET_SWWDT3:
      MEMFAULT_PRINT_RESET_INFO(" Software Watchdog");
      reset_reason = kMfltRebootReason_SoftwareWatchdog;
      break;

    case CY_SYSLIB_RESET_CSV_LOSS_WAKEUP:
    case CY_SYSLIB_RESET_CSV_ERROR_WAKEUP:
      MEMFAULT_PRINT_RESET_INFO(" Clock-Supervision Logic Reset");
      reset_reason = kMfltRebootReason_ClockFailure;
      break;

    case CY_SYSLIB_RESET_HIB_WAKEUP:
      MEMFAULT_PRINT_RESET_INFO(" Hibernation Wakeup");
      reset_reason = kMfltRebootReason_LowPower;
      break;

#ifdef CY_IP_M7CPUSS
    case CY_SYSLIB_RESET_XRES:
    case CY_SYSLIB_RESET_BODVDDD:
    case CY_SYSLIB_RESET_BODVDDA:
    case CY_SYSLIB_RESET_BODVCCD:
      MEMFAULT_PRINT_RESET_INFO(" Brown Out");
      reset_reason = kMfltRebootReason_BrownOutReset;
      break;

    case CY_SYSLIB_RESET_OVDVDDD:
    case CY_SYSLIB_RESET_OVDVDDA:
    case CY_SYSLIB_RESET_OVDVCCD:
    case CY_SYSLIB_RESET_OCD_ACT_LINREG:
    case CY_SYSLIB_RESET_OCD_DPSLP_LINREG:
    case CY_SYSLIB_RESET_OCD_REGHC:
    case CY_SYSLIB_RESET_PMIC:
    case CY_SYSLIB_RESET_PXRES:
    case CY_SYSLIB_RESET_STRUCT_XRES:
      reset_reason = kMfltRebootReason_UnknownError;
      break;
    case CY_SYSLIB_RESET_PORVDDD:
      MEMFAULT_PRINT_RESET_INFO(" Power on Reset");
      reset_reason = kMfltRebootReason_PowerOnReset;
      break;
#endif
    default:
      reset_reason = kMfltRebootReason_Unknown;
      MEMFAULT_PRINT_RESET_INFO(" Unknown");
      break;

  }

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}

void memfault_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  memfault_reboot_reason_get(&reset_info);
  memfault_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}
