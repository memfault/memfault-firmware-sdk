#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/components.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Default ESP-IDF implementation of the memfault platform device info API.
//! Uses WiFi MAC address for device serial, other values are set from Kconfig.
//!
//! @note the first time this function is called must be from a non-interrupt
//! context (i.e. when it's called from 'memfault_boot()' during system
//! initialization), because it accesses the eFuse to read the MAC address.
void memfault_esp_port_get_device_info(struct MemfaultDeviceInfo *info);

#ifdef __cplusplus
}
#endif
