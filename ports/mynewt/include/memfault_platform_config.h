#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1

//! Override the default names for configuration files so application developer
//! can easily customize without having to modify the port
#define MEMFAULT_TRACE_REASON_USER_DEFS_FILE  "memfault_trace_reason_mynewt_config.def"
#define MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE "memfault_metrics_heartbeat_mynewt_config.def"

#ifdef __cplusplus
}
#endif
