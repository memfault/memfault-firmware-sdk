#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#ifdef __cplusplus
extern "C" {
#endif

//! A placeholder for initializing reboot tracking
//!
//! @note A user of the SDK can chose to implement this function and call it during bootup.
//!
//! @note A reference implementation can be found in the NRF52 demo application:
//!  platforms/nrf5/libraries/memfault/platform_reference_impl/memfault_platform_reboot_tracking.c
void memfault_platform_reboot_tracking_boot(void);

#ifdef __cplusplus
}
#endif
