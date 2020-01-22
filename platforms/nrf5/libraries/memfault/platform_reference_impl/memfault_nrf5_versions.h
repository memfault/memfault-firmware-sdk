#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs which implement default software version handlers that work with the NRF52 SDK



void memfault_get_nrf5_app_version(char *buf, size_t buf_len);

void memfault_get_nrf5_bluetooth_version(char *buf, size_t buf_len);

void memfault_get_nrf5_bootloader_version(char *buf, size_t buf_len);
