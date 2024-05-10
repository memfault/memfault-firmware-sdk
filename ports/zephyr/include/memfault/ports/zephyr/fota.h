#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#if defined(__cplusplus)
extern "C" {
#endif

//! @brief Initiates a FOTA update for Zephyr systems
//!
//! @note This function will block until the FOTA update is complete. If the update is successful,
//! the device will be rebooted to apply the update.
//!
//! @return
//!   < 0 Error while trying to perform the FOTA update
//!     0 Check completed successfully - No new update available
int memfault_zephyr_fota_start(void);

#if defined(__cplusplus)
}
#endif
