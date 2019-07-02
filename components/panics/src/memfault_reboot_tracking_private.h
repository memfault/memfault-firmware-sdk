#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal utilities used for tracking reboot reasons

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Note: Must be kept in sync with the python enum at www/database/constants.py
//! TODO: Autogenerate the python & C header for this enum from a shared json file
typedef enum MfltRebootReason {
  kMfltRebootReason_Unknown = 0x0,
  kMfltRebootReason_Assert = 0x8001,
  kMfltRebootReason_Watchdog = 0x8002,
  kMfltRebootReason_UsageFault = 0x8002,
  kMfltRebootReason_BusFault = 0x9100,
  kMfltRebootReason_MemFault = 0x9200,
  kMfltRebootReason_HardFault = 0x9400,
} eMfltRebootReason;

typedef struct MfltCrashInfo {
  eMfltRebootReason reason;
  uint32_t pc;
  uint32_t lr;
} sMfltCrashInfo;

//! Flag that a reboot is about to take place due to a crash
//!
//! It's expected this is called right before issuing a reboot due to a crash
//! such as a Hardfault or Assert
//!
//! If three reboots of this type happen in a row, the bootloader will enter recovery
//! mode, ready to perform a OTA update to recover the device
void memfault_reboot_tracking_mark_crash(const sMfltCrashInfo *info);

//! Retrieves any saved crash information
bool memfault_reboot_tracking_get_crash_info(sMfltCrashInfo *info);

//! Clears any crash information which was stored
void memfault_reboot_tracking_clear_crash_info(void);

//! Return true if the firmware has crashed three times without a call to
//! 'memfault_mark_system_stable'
bool memfault_reboot_tracking_is_firmware_unstable(void);

//! Invoked by the bootloader when it is about to launch an App
//!
//! If the app is launched three times in a row without any acknowledgment from the app itself, the
//! bootloader will enter recovery mode to perform an OTA update
void memfault_reboot_tracking_mark_app_launch_attempted(void);

#ifdef __cplusplus
}
#endif
