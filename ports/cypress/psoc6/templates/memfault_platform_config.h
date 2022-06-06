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

#ifdef __cplusplus
}
#endif
