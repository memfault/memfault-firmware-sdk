#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#ifdef __cplusplus
extern "C" {
#endif

//! This function should be called just before entering deep sleep
//! (esp_deep_sleep_start()). It saves Memfault system state to RTC memory so it
//! can be restored after waking up from deep sleep.
void memfault_platform_deep_sleep_save_state(void);

//! This function should be called after waking up from deep sleep, before
//! calling memfault_boot(). It restores state saved in
//! memfault_platform_deep_sleep_save_state().
void memfault_platform_deep_sleep_restore_state(void);

#ifdef __cplusplus
}
#endif
