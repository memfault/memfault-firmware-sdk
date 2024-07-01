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

//! @brief A callback invoked after a download has completed
//!
//! @note The default implementation will reboot the system after an OTA download has completed.
//! A boolean return value is added to allow for custom implementations that may want to handle the
//! download completion differently (e.g. in a testing framework context where returning from the
//! FOTA will allow the result of the image download to be captured).
//!
//! A custom implementation can be provided by setting
//! CONFIG_MEMFAULT_ZEPHYR_FOTA_DOWNLOAD_CALLBACK_CUSTOM=y and implementing this function
//!
//! @return true if the download was successful, false otherwise
bool memfault_zephyr_fota_download_callback(void);

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
