#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Defines configuration specific to PSOC6 Port
//!
//! These options can be changed from the DEFINES section in a MTB Makefile, i.e
//! DEFINES+=MEMFAULT_PORT_MEMORY_TRACKING_ENABLED=0 or overriden with a custom
//! "memfault_platform_config.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Enable collection of metrics around heap utilization using heartbeat metrics

#ifndef MEMFAULT_PORT_MEMORY_TRACKING_ENABLED
#define MEMFAULT_PORT_MEMORY_TRACKING_ENABLED 1
#endif

//! Enables collection of statistics around Wi-Fi using heartbeat metrics
#ifndef MEMFAULT_PORT_WIFI_TRACKING_ENABLED
#define MEMFAULT_PORT_WIFI_TRACKING_ENABLED 1
#endif

#ifdef __cplusplus
}
#endif
