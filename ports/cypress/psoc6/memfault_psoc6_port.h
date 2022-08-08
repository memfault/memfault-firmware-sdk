#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! psoc6 specific port APIs

#ifdef __cplusplus
extern "C" {
#endif

//! Tracks Wi-Fi Connection Manager (wcm) subsytem with memfault heartbeats
//!
//! @note: This must be called after cy_wcm_init() is called
void memfault_wcm_metrics_boot(void);

#ifdef __cplusplus
}
#endif
