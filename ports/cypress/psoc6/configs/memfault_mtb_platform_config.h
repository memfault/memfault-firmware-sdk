#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Default PSOC6 configuration

#ifdef __cplusplus
extern "C" {
#endif

//! Pick up logging macros from memfault_platform_log_config.h
#define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1

//! Change the name of the coredump noinit section to align with default
//! PSOC6 .ld of KEEP(*(.noinit))
#define MEMFAULT_PLATFORM_COREDUMP_NOINIT_SECTION_NAME ".mflt_coredump.noinit"

#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 8192

#define MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE "ports/cypress/psoc6/configs/memfault_metrics_mtb_heartbeat_config.def"

// Include users config if present
#if __has_include("memfault_platform_config.h")
    #include "memfault_platform_config.h"
#endif

//! Defines specific to PSOC6 Port
//! Note that we include them last so an end user settings above override the defaults.
#include "psoc6_default_config.h"

#ifdef __cplusplus
}
#endif
