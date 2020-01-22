#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Zephyr-port specific http utility for interfacing with http

#ifdef __cplusplus
extern "C" {
#endif

int memfault_zephyr_port_post_data(void);

#ifdef __cplusplus
}
#endif
