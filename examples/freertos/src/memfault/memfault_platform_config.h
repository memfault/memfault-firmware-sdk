//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Application platform config. Add Memfault configs here.

#pragma once

#define MEMFAULT_USE_GNU_BUILD_ID 1
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 8192
#define MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS 1
#define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 1
#define MEMFAULT_FREERTOS_PORT_HEAP_STATS_ENABLE 1
#define MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE 0
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 60
#define MEMFAULT_COMPACT_LOG_ENABLE 1

#define MEMFAULT_COLLECT_MPU_STATE 1

#define MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE 1

// Enable adding custom demo shell commands
#define MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS 1

// Provide a custom runtime-configurable project key
#define MEMFAULT_MESSAGE_HEADER_CONTAINS_PROJECT_KEY 1
extern const char g_memfault_project_key[33];
#define MEMFAULT_PROJECT_KEY g_memfault_project_key

#if !defined(MEMFAULT_PLATFORM_HAS_LOG_CONFIG)
  #define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1
#endif
