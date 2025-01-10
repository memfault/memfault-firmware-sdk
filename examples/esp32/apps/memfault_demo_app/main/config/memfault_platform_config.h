#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#define MEMFAULT_TASK_WATCHDOG_ENABLE 1

//! Disable the default set of built-in FreeRTOS thread metrics. ESP-IDF links
//! the application from a set of static libraries, so we are unable to override
//! the weak definition of the default thread metrics
//! (MEMFAULT_METRICS_DEFINE_THREAD_METRICS()). Instead, explicitly disable the
//! defaults here and define a custom set of thread metrics in the application.
#define MEMFAULT_METRICS_THREADS_DEFAULTS_INDEX 0
#define MEMFAULT_METRICS_THREADS_DEFAULTS 0
