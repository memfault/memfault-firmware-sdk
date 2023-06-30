#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief Memfault Zephyr log backend APIs

#ifdef __cplusplus
extern "C" {
#endif

//! Disable the Memfault Zephyr log backend. Only valid after the backend has
//! been initialized.
void memfault_zephyr_log_backend_disable(void);

#ifdef __cplusplus
}
#endif
