//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! A port for recovering reset reason information on the SiWx91x family.
//!
//! The SiWx91x chips only support 2 distinct reset reasons:
//!  - Power On Reset
//!  - Pin Reset
//!
//! When neither is active, we assume a generic software reset.
//!
//! See "9.10.32 MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS" and "9.10.46
//! MCU_FSM_WAKEUP_STATUS_REG" of the SiWx917 Family Reference Manual, which was
//! the reference for this implementation.
//!
//! NOTE: this requires the user to load the global variable
//! "memfault_siwx91x_fsm_wakeup_status_reg" with the contents of
//! MCU_FSM_WAKEUP_STATUS_REG before calling SystemInit() on boot! Example:
//!
//! #include "si91x_device.h"
//! void Reset_Handler(void) {
//!   // Load the global variable with the contents of MCU_FSM_WAKEUP_STATUS_REG
//!   extern uint32_t memfault_siwx91x_fsm_wakeup_status_reg;
//!   memfault_siwx91x_fsm_wakeup_status_reg = MCU_FSM->MCU_FSM_WAKEUP_STATUS_REG;
//!   // Call SystemInit() to initialize the system
//!   SystemInit();
//!   // Call the main function
//!   main();
//! }

#include <inttypes.h>

#include "memfault/core/debug_log.h"
#include "memfault/ports/reboot_reason.h"

#if MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
  #define MEMFAULT_PRINT_RESET_INFO(...) MEMFAULT_LOG_INFO(__VA_ARGS__)
#else
  #define MEMFAULT_PRINT_RESET_INFO(...)
#endif

// These bitmasks don't exist in the WiseConnect SDK, define them here.
// See "9.10.46 MCU_FSM_WAKEUP_STATUS_REG" in the siw917x-family-rm .
#define MCU_FIRST_POWERUP_RESET_N_MASK (0x1 << 17)
#define MCU_FIRST_POWERUP_POR_MASK (0x1 << 16)

// This register value needs to be cached by the system before SystemInit() is
// called:
// https://github.com/SiliconLabs/wiseconnect/blob/v3.4.1/components/device/silabs/si91x/mcu/core/chip/src/system_si91x.c#L280-L281
uint32_t memfault_siwx91x_fsm_wakeup_status_reg;

void memfault_reboot_reason_get(sResetBootupInfo *info) {
  eMemfaultRebootReason reset_reason;

  MEMFAULT_PRINT_RESET_INFO("Reset Reason Reg: %" PRIx32, memfault_siwx91x_fsm_wakeup_status_reg);
  char *reset_reason_str;
  (void)reset_reason_str;

  if (!(memfault_siwx91x_fsm_wakeup_status_reg & MCU_FIRST_POWERUP_POR_MASK)) {
    // If the POR bit was cleared, this is a POR
    reset_reason = kMfltRebootReason_PowerOnReset;
    reset_reason_str = "Power On Reset";
  } else if (!(memfault_siwx91x_fsm_wakeup_status_reg & MCU_FIRST_POWERUP_RESET_N_MASK)) {
    // If the First Powerup Reset bit is cleared, this was a reset pin reset
    reset_reason = kMfltRebootReason_PinReset;
    reset_reason_str = "Pin Reset";
  } else {
    // Otherwise, if both bits were set, this is a soft reset
    reset_reason_str = "Software Reset";
    reset_reason = kMfltRebootReason_SoftwareReset;
  }

  MEMFAULT_PRINT_RESET_INFO("Reset Cause: %s", reset_reason_str);

  *info = (sResetBootupInfo){
    .reset_reason_reg = memfault_siwx91x_fsm_wakeup_status_reg,
    .reset_reason = reset_reason,
  };
}
