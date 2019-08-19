#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault OTA has a subsystem used to track the stability of the main application. If the main
//! app fails to start (no call is made to 'memfault_reboot_tracking_mark_system_started') or
//! crashes multiple times in a row (no call made to 'memfault_reboot_tracking_mark_system_stable')
//! the bootloader will stop attempting to launch the application and enter over-the-air mode.
//! Once in OTA mode, a firmware update can be performed to recover the system.
//!
//! By leveraging this module, one can prevent a buggy main firmware from bricking the device

#define MEMFAULT_REBOOT_TRACKING_REGION_SIZE 256

#ifdef __cplusplus
extern "C" {
#endif

//! Set's the memory region used for reboot tracking.
//!
//! @note
//!   - This region should _not_ be initialized on bootup by the main application
//!   - The size of the region should be MEMFAULT_REBOOT_TRACKING_REGION_SIZE
//!   - This should be called once on bootup of the system prior to making
//!     any other reboot_tracking calls
int memfault_reboot_tracking_boot(void *start_addr);

//! Marks the firmware as stable
//!
//! Resets the count which is used to track if the firmware is boot looping
void memfault_reboot_tracking_mark_system_stable(void);

//! Should be called as soon as the system firmware starts
//!
//! The bootloader uses this info to conclude the app is launching successfully.
void memfault_reboot_tracking_mark_system_started(void);

#ifdef __cplusplus
}
#endif
