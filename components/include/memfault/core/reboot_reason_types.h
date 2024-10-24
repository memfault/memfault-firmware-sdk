#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Different types describing information collected as part of "Trace Events"

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/config.h"
#include "memfault/core/compiler.h"

#define MEMFAULT_REBOOT_REASON_EXPECTED_CUSTOM_BASE 0x1000
#define MEMFAULT_REBOOT_REASON_EXPECTED_CUSTOM_MAX 0x1100

#define MEMFAULT_REBOOT_REASON_UNEXPECTED_CUSTOM_BASE 0xA000
#define MEMFAULT_REBOOT_REASON_UNEXPECTED_CUSTOM_MAX 0xA100

#undef MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE
#undef MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE

//! This enum must be packed to prevent compiler optimizations in instructions which load an
//! eMemfaultRebootReason.
typedef enum MEMFAULT_PACKED MfltResetReason {
  // A reboot reason was not determined either by hardware or a previously marked reboot reason
  // This reason is classified as a crash when calculating the operational_crashfree_hours metric
  kMfltRebootReason_Unknown = 0x0000,

  //
  // Expected Resets
  //

  kMfltRebootReason_UserShutdown = 0x0001,
  kMfltRebootReason_UserReset = 0x0002,
  kMfltRebootReason_FirmwareUpdate = 0x0003,
  kMfltRebootReason_LowPower = 0x0004,
  kMfltRebootReason_DebuggerHalted = 0x0005,
  kMfltRebootReason_ButtonReset = 0x0006,
  kMfltRebootReason_PowerOnReset = 0x0007,
  kMfltRebootReason_SoftwareReset = 0x0008,

  // MCU went through a full reboot due to exit from lowest power state
  kMfltRebootReason_DeepSleep = 0x0009,
  // MCU reset pin was toggled
  kMfltRebootReason_PinReset = 0x000A,
  // Self Test generated reboot
  kMfltRebootReason_SelfTest = 0x000B,

//
// User Defined Expected Resets
//

#if MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE == 1
  kMfltRebootReason_ExpectedBase = MEMFAULT_REBOOT_REASON_EXPECTED_CUSTOM_BASE,
  #define MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE(name) kMfltRebootReason_##name,
  #define MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE(name)
  #include MEMFAULT_REBOOT_REASON_USER_DEFS_FILE
  #undef MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE
  #undef MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE
#endif

  //
  // Unexpected Resets
  //

  // Can be used to flag an unexpected reset path. i.e NVIC_SystemReset()
  // being called without any reboot logic getting invoked.
  kMfltRebootReason_UnknownError = 0x8000,
  kMfltRebootReason_Assert = 0x8001,

  // Deprecated in favor of HardwareWatchdog & SoftwareWatchdog. This way,
  // the amount of watchdogs not caught by software can be easily tracked
  kMfltRebootReason_WatchdogDeprecated = 0x8002,

  kMfltRebootReason_BrownOutReset = 0x8003,
  kMfltRebootReason_Nmi = 0x8004,  // Non-Maskable Interrupt

  // More details about nomenclature in https://mflt.io/root-cause-watchdogs
  kMfltRebootReason_HardwareWatchdog = 0x8005,
  kMfltRebootReason_SoftwareWatchdog = 0x8006,

  // A reset triggered due to the MCU losing a stable clock. This can happen,
  // for example, if power to the clock is cut or the lock for the PLL is lost.
  kMfltRebootReason_ClockFailure = 0x8007,

  // A software reset triggered when the OS or RTOS end-user code is running on top of identifies
  // a fatal error condition.
  kMfltRebootReason_KernelPanic = 0x8008,

  // A reset triggered when an attempt to upgrade to a new OTA image has failed and a rollback
  // to a previous version was initiated
  kMfltRebootReason_FirmwareUpdateError = 0x8009,

  // A software reset triggered due to a dynamic memory (heap) allocation failure.
  kMfltRebootReason_OutOfMemory = 0x800A,
  // A reset due to stack overflow
  kMfltRebootReason_StackOverflow = 0x800B,

  // Resets from Arm Faults
  kMfltRebootReason_BusFault = 0x9100,
  kMfltRebootReason_MemFault = 0x9200,
  kMfltRebootReason_UsageFault = 0x9300,
  kMfltRebootReason_HardFault = 0x9400,
  // A reset which is triggered when the processor faults while already
  // executing from a fault handler.
  kMfltRebootReason_Lockup = 0x9401,

  // A reset triggered due to a security violation
  kMfltRebootReason_SecurityViolation = 0x9402,

  // A reset triggered due to a parity error (i.e. memory integrity check)
  kMfltRebootReason_ParityError = 0x9403,

  // A reset triggered due to a temperature error
  kMfltRebootReason_Temperature = 0x9404,

  // A reset due to some other hardware error
  kMfltRebootReason_Hardware = 0x9405,

//
// User Defined Unexpected Resets
//

#if MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE == 1
  kMfltRebootReason_UnexpectedBase = MEMFAULT_REBOOT_REASON_UNEXPECTED_CUSTOM_BASE,
  #define MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE(name)
  #define MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE(name) kMfltRebootReason_##name,
  #include MEMFAULT_REBOOT_REASON_USER_DEFS_FILE
  #undef MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE
  #undef MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE
#endif

} eMemfaultRebootReason;

MEMFAULT_STATIC_ASSERT(sizeof(eMemfaultRebootReason) == 2, "Reboot reason enum must be 2 bytes");

#if MEMFAULT_REBOOT_REASON_CUSTOM_ENABLE == 1
  // Ensure that the custom reboot reasons are within the expected range
  #define MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE(name)                                            \
    MEMFAULT_STATIC_ASSERT(kMfltRebootReason_##name < MEMFAULT_REBOOT_REASON_EXPECTED_CUSTOM_MAX, \
                           "User defined expected reboot is out of range");
  #define MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE(name)                       \
    MEMFAULT_STATIC_ASSERT(                                                    \
      kMfltRebootReason_##name < MEMFAULT_REBOOT_REASON_UNEXPECTED_CUSTOM_MAX, \
      "User defined unexpected reboot is out of range");
  #include MEMFAULT_REBOOT_REASON_USER_DEFS_FILE
  #undef MEMFAULT_EXPECTED_REBOOT_REASON_DEFINE
  #undef MEMFAULT_UNEXPECTED_REBOOT_REASON_DEFINE
#endif

#ifdef __cplusplus
}
#endif
