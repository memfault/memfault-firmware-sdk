#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Different types describing information collected as part of "Trace Events"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MfltResetReason {
  kMfltRebootReason_Unknown = 0x0000,

  // Normal Resets
  kMfltRebootReason_UserShutdown = 0x0001,
  kMfltRebootReason_UserReset = 0x0002,
  kMfltRebootReason_FirmwareUpdate = 0x0003,
  kMfltRebootReason_LowPower = 0x0004,

  // Error Resets
  kMfltRebootReason_UnknownError = 0x8000,
  kMfltRebootReason_Assert = 0x8001,
  kMfltRebootReason_Watchdog = 0x8002,

  // Arm Faults
  kMfltRebootReason_BusFault = 0x9100,
  kMfltRebootReason_MemFault = 0x9200,
  kMfltRebootReason_UsageFault = 0x9300,
  kMfltRebootReason_HardFault = 0x9400,
} eMfltResetReason;

#ifdef __cplusplus
}
#endif
