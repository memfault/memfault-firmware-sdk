#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Default configuration settings for the Memfault SDK
//! This file should always be picked up through "memfault/config.h"
//! and never included directly

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MEMFAULT_PLATFORM_CONFIG_INCLUDE_DEFAULTS
#error "Include "memfault/config.h" instead of "memfault/default_config.h" directly"
#endif

//
// Core Components
//

//! Controls the use of the Gnu build ID feature.
#ifndef MEMFAULT_USE_GNU_BUILD_ID
#define MEMFAULT_USE_GNU_BUILD_ID 0
#endif

//! Controls the name used to reference the GNU build ID data
#ifndef MEMFAULT_GNU_BUILD_ID_SYMBOL
  #define MEMFAULT_GNU_BUILD_ID_SYMBOL __start_gnu_build_id_start
#endif

//! Allows users to dial in the correct amount of storage for their
//! software version + build ID string.
#ifndef MEMFAULT_UNIQUE_VERSION_MAX_LEN
#define MEMFAULT_UNIQUE_VERSION_MAX_LEN 32
#endif

//! While it is recommended that MEMFAULT_SDK_ASSERT be left enabled, they can
//! be disabled by adding -DMEMFAULT_SDK_ASSERT_ENABLED=0 to the CFLAGS used to
//! compile the SDK
#ifndef MEMFAULT_SDK_ASSERT_ENABLED
#define MEMFAULT_SDK_ASSERT_ENABLED 1
#endif

//! Controls whether or not memfault_platform_halt_if_debugging() will be called
//! at the beginning of an assert handler prior to trapping into the fault handler.
//!
//! This can make viewing the stack trace prior to exception easier when debugging locally
#ifndef MEMFAULT_ASSERT_HALT_IF_DEBUGGING_ENABLED
#define MEMFAULT_ASSERT_HALT_IF_DEBUGGING_ENABLED 0
#endif

//! Install a low-level hook into the C stdlib assert() library call, to capture
//! coredumps when asserts occur.
#ifndef MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED
#define MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED 1
#endif

//! Controls whether or not device serial is encoded in events
//!
//! When disabled (default), the device serial is derived from the API route
//! used to post data to the cloud
#ifndef MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL
#define MEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL 0
#endif

//! Controls whether or not the Build Id is encoded in events
//!
//! The Build Id can be used by Memfault to reliably find the corresponding
//! symbol file to process the event. When disabled, the software version & type
//! are used instead to find the symbol file.
#ifndef MEMFAULT_EVENT_INCLUDE_BUILD_ID
#define MEMFAULT_EVENT_INCLUDE_BUILD_ID 1
#endif

//! Controls whether or not the Build Id is encoded in coredumps
//!
//! The Build Id can be used by Memfault to reliably find the corresponding
//! symbol file while processing a coredump. When disabled, the software version & type
//! are used instead to find the symbol file.
#ifndef MEMFAULT_COREDUMP_INCLUDE_BUILD_ID
#define MEMFAULT_COREDUMP_INCLUDE_BUILD_ID 1
#endif

//! Controls the truncation of the Build Id that is encoded in events
//!
//! The full Build Id hash is 20 bytes, but is truncated by default to save space. The
//! default truncation to 6 bytes (48 bits) has a 0.1% chance of collisions given
//! 7.5 * 10 ^ 5 (750,000) Build Ids.
//! See https://en.wikipedia.org/wiki/Birthday_problem#Probability_table
#ifndef MEMFAULT_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES
#define MEMFAULT_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES 6
#endif

//! Controls whether or not run length encoding (RLE) is used when packetizing
//! data
//!
//! Significantly reduces transmit size of data with repeated patterns such as
//! coredumps at the cost of ~1kB of codespace
#ifndef MEMFAULT_DATA_SOURCE_RLE_ENABLED
#define MEMFAULT_DATA_SOURCE_RLE_ENABLED 1
#endif

//! Controls default log level that will be saved to https://mflt.io/logging
#ifndef MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL
#define MEMFAULT_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL kMemfaultPlatformLogLevel_Info
#endif

//! Controls whether or not to include the memfault_log_trigger_collection() API
//! and the module that is responsible for sending collected logs.
#ifndef MEMFAULT_LOG_DATA_SOURCE_ENABLED
#define MEMFAULT_LOG_DATA_SOURCE_ENABLED 1
#endif

//! Maximum length of a compact log export chunk.
//!
//! Controls the maximum chunk length when exporting compact logs with memfault_log_export.
#ifndef MEMFAULT_LOG_EXPORT_CHUNK_MAX_LEN
  #define MEMFAULT_LOG_EXPORT_CHUNK_MAX_LEN 80
#endif

//! Maximum length a log record can occupy
//!
//! Structs holding this log may be allocated on the stack so care should be taken
//! to make the size is not to large to blow through the allocated space for the stack.
#ifndef MEMFAULT_LOG_MAX_LINE_SAVE_LEN
#define MEMFAULT_LOG_MAX_LINE_SAVE_LEN 128
#endif

//! Control whether or automatic persisting of MEMFAULT_LOG_*'s is enabled
#ifndef MEMFAULT_SDK_LOG_SAVE_DISABLE
#define MEMFAULT_SDK_LOG_SAVE_DISABLE 0
#endif

// Allows for the MEMFAULT_LOG APIs to be re-mapped to another macro rather
// than the  memfault_platform_log dependency function
//
// In this mode a user of the SDK will need to define the following macros:
//   MEMFAULT_LOG_DEBUG(...)
//   MEMFAULT_LOG_INFO(...)
//   MEMFAULT_LOG_WARN(...)
//   MEMFAULT_LOG_ERROR(...)
//
// And if the "demo" component is being used:
//   MEMFAULT_LOG_RAW(...)
#ifndef MEMFAULT_PLATFORM_HAS_LOG_CONFIG
#define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 0
#endif

//! Controls whether or not multiple events will be batched into a single
//! message when reading information via the event storage data source.
//!
//! This can be a useful strategy when trying to maximize the amount of data
//! sent in a single transmission unit.
//!
//! To enable, you will need to update the compiler flags for your project, i.e
//!   CFLAGS += -DMEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED=1
#ifndef MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED
#define MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED 0
#endif

//! Enables support for non-volatile event storage. At run-time the non-volatile
//! storage must be enabled before use. See nonvolatile_event_storage.h for
//! details.
#ifndef MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED
#define MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED 0
#endif

#if MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED != 0

//! When batching is enabled, controls the maximum amount of event data bytes
//! that will be in a single message.
//!
//! By default, there is no limit. To set a limit you will need to update the
//! compiler flags for your project the following would cap the data payload
//! size at 1024 bytes
//!  CFLAGS += -DMEMFAULT_EVENT_STORAGE_READ_BATCHING_MAX_BYTES=1024
#ifndef MEMFAULT_EVENT_STORAGE_READ_BATCHING_MAX_BYTES
#define MEMFAULT_EVENT_STORAGE_READ_BATCHING_MAX_BYTES UINT32_MAX
#endif

#endif /* MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED */

//! The max size of a chunk. Should be a size suitable to write to transport
//! data is being dumped over.
#ifndef MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN
#define MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN 80
#endif

#ifndef MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS
#define MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS 0
#endif

//
// Heap Statistics Configuration
//

//! When the FreeRTOS port is being used, controls whether or not heap
//! allocation tracking is enabled.
//! Note: When using this feature, MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE 0
//! is also required
#ifndef MEMFAULT_FREERTOS_PORT_HEAP_STATS_ENABLE
#define MEMFAULT_FREERTOS_PORT_HEAP_STATS_ENABLE 0
#endif

//! Enable this flag to collect the heap stats information on coredump
#ifndef MEMFAULT_COREDUMP_COLLECT_HEAP_STATS
#define MEMFAULT_COREDUMP_COLLECT_HEAP_STATS 0
#endif

//! Max number of recent outstanding heap allocations to track.
//! oldest tracked allocations are expired (by allocation order)
#ifndef MEMFAULT_HEAP_STATS_MAX_COUNT
#define MEMFAULT_HEAP_STATS_MAX_COUNT 32
#endif

//! Controls whether or not memfault_lock() will be used in the heap stats module.  If the
//! allocation implementation in use already enables locks of it's own (i.e FreeRTOS heap_*.c
//! implementations), the recommendation is to disable memfault locking
#ifndef MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE
#define MEMFAULT_COREDUMP_HEAP_STATS_LOCK_ENABLE 1
#endif

//
// Task Watchdog Configuration
//

//! Use this flag to enable the task watchdog component. See the header file at
//! components/include/memfault/core/task_watchdog.h for usage details.
#ifndef MEMFAULT_TASK_WATCHDOG_ENABLE
#define MEMFAULT_TASK_WATCHDOG_ENABLE 0
#endif

//! Configure the task watchdog timeout value in milliseconds.
#ifndef MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS
#define MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS 1000
#endif

//! Enable this flag to collect the task watchdog information on coredump. Only
//! needed if the entire memory contents are not already collected on coredump.
#ifndef MEMFAULT_COREDUMP_COLLECT_TASK_WATCHDOG_REGION
#define MEMFAULT_COREDUMP_COLLECT_TASK_WATCHDOG_REGION 0
#endif

//
// Trace Configuration
//

//! Set the name for the user trace reason config file, which is where custom
//! trace reasons can be defined
#ifndef MEMFAULT_TRACE_REASON_USER_DEFS_FILE
#define MEMFAULT_TRACE_REASON_USER_DEFS_FILE \
  "memfault_trace_reason_user_config.def"
#endif

//! By default, the Memfault SDK allows for trace events with logs to be
//! captured for trace events from ISRs.
//!
//! However, this can be disabled to reduce static RAM usage by
//! MEMFAULT_TRACE_EVENT_MAX_LOG_LEN
#ifndef MEMFAULT_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
#define MEMFAULT_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED 1
#endif

//! The maximum size allocated for a trace event log
#ifndef MEMFAULT_TRACE_EVENT_MAX_LOG_LEN
#define MEMFAULT_TRACE_EVENT_MAX_LOG_LEN 80
#endif

//! Enables use of "compact" logs.
//!
//! Compact logs convert format strings to an integer index at compile time and serialize an "id"
//! and format arguments on the device. The Memfault cloud will decode the log and format the full
//! log greatly reducing the storage and overhead on the device side.
#ifndef MEMFAULT_COMPACT_LOG_ENABLE
#define MEMFAULT_COMPACT_LOG_ENABLE 0
#endif

//
// Metrics Component Configurations
//

//! The frequency in seconds to collect heartbeat metrics. The suggested
//! interval is once per hour but the value can be overriden to be as low as
//! once every 15 minutes.
#ifndef MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 3600
#endif

#ifndef MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#define MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE \
  "memfault_metrics_heartbeat_config.def"
#endif

//! Enable the sync_successful metric API
#ifndef MEMFAULT_METRICS_SYNC_SUCCESS
#define MEMFAULT_METRICS_SYNC_SUCCESS 0
#endif

//! Enable the sync_memfault_successful metric API
#ifndef MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS
#define MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS 0
#endif

//! Enable the connectivity_connected_time_ms metric API
#ifndef MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME
#define MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME 0
#endif

//! Enable the battery metrics API
#ifndef MEMFAULT_METRICS_BATTERY_ENABLE
#define MEMFAULT_METRICS_BATTERY_ENABLE 0
#endif

//
// Panics Component Configs
//

// By default, enable collection of interrupt state at the time the coredump is
// collected. User of the SDK can disable collection by adding the following
// compile flag: -DMEMFAULT_COLLECT_INTERRUPT_STATE=0
//
// NB: The default Interrupt collection configuration requires ~150 bytes of
// coredump storage space to save.
#ifndef MEMFAULT_COLLECT_INTERRUPT_STATE
#define MEMFAULT_COLLECT_INTERRUPT_STATE 1
#endif

// ARMv7-M supports at least 32 external interrupts in the NVIC and a maximum of
// 496.
//
// By default, only collect the minimum set. For a typical application this will
// generally cover all the interrupts in use.
//
// If there are more interrupts implemented than analyzed a note will appear in
// the "ISR Analysis" tab for the Issue analyzed in the Memfault UI about the
// appropriate setting needed.
//
// NB: For each additional 32 NVIC interrupts analyzed, 40 extra bytes are
// needed for coredump storage.
#ifndef MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT
#define MEMFAULT_NVIC_INTERRUPTS_TO_COLLECT 32
#endif

// Enables the collection of fault registers information
//
// When fault registers are collected, an analysis will be presented on the
// "issues" page of the Memfault UI.
#ifndef MEMFAULT_COLLECT_FAULT_REGS
#define MEMFAULT_COLLECT_FAULT_REGS 1
#endif

// ARMv7-M can support an IMPLEMENTATION DEFINED number of MPU regions if the MPU
// is implemented. If MPU_TYPE register is non-zero then the MPU is implemented and the
// number of regions will be indicated in MPU_TYPE[DREGIONS] since the ARMv7-m
// uses a unified MPU, e.g. MPU_TYPE[SEPARATE] = 0.
//
// By default, we disable MPU collection since it requires more RAM and not
// every MCU has an MPU implemented or, if it is, may not be employed. Users
// should set MEMFAULT_COLLECT_MPU_STATE and verify MEMFAULT_MPU_REGIONS_TO_COLLECT
// matches their needs to collect the number of regions they actually configure
// in the MPU.
//
// For ideas on how to make use of the MPU in your application check out
// https://mflt.io/mpu-stack-overflow-debug
#ifndef MEMFAULT_COLLECT_MPU_STATE
// This controls allocations and code need to collect the MPU state.
#define MEMFAULT_COLLECT_MPU_STATE 0
#endif

#ifndef MEMFAULT_CACHE_FAULT_REGS
// Controls whether memfault_coredump_get_arch_regions() will collect
// the HW registers as-is insert a pre-cached memory copy of them.
// This entails calling memfault_coredump_cache_fault_regs() before
// the OS processes the fault.
#define MEMFAULT_CACHE_FAULT_REGS 0
#endif

#ifndef MEMFAULT_MPU_REGIONS_TO_COLLECT
// This is used to define the sMfltMpuRegs structure.
#define MEMFAULT_MPU_REGIONS_TO_COLLECT 8
#endif

// By default, exception handlers use CMSIS naming conventions. By default, the
// CMSIS library provides a weak implementation for each handler that is
// implemented as an infinite loop. By using the same name, the Memfault SDK can
// override this default behavior to capture crash information when a fault
// handler runs.
//
// However, if needed, each handler can be renamed using the following
// preprocessor defines:

#ifndef MEMFAULT_EXC_HANDLER_HARD_FAULT
#define MEMFAULT_EXC_HANDLER_HARD_FAULT HardFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT
#define MEMFAULT_EXC_HANDLER_MEMORY_MANAGEMENT MemoryManagement_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_BUS_FAULT
#define MEMFAULT_EXC_HANDLER_BUS_FAULT BusFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_USAGE_FAULT
#define MEMFAULT_EXC_HANDLER_USAGE_FAULT UsageFault_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_NMI
#define MEMFAULT_EXC_HANDLER_NMI NMI_Handler
#endif

#ifndef MEMFAULT_EXC_HANDLER_WATCHDOG
#define MEMFAULT_EXC_HANDLER_WATCHDOG MemfaultWatchdog_Handler
#endif

#ifndef MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM
#define MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM  0
#endif

// If enabled, memfault_fault_handler will return back to OS
// exception handling code instead of rebooting the device.
#ifndef MEMFAULT_FAULT_HANDLER_RETURN
#define MEMFAULT_FAULT_HANDLER_RETURN 0
#endif

//
// Http Configuration Options
//

#ifndef MEMFAULT_HTTP_CHUNKS_API_HOST
#define MEMFAULT_HTTP_CHUNKS_API_HOST "chunks.memfault.com"
#endif

#ifndef MEMFAULT_HTTP_DEVICE_API_HOST
#define MEMFAULT_HTTP_DEVICE_API_HOST "device.memfault.com"
#endif

#ifndef MEMFAULT_HTTP_APIS_DEFAULT_PORT
#define MEMFAULT_HTTP_APIS_DEFAULT_PORT (443)
#endif

#ifndef MEMFAULT_HTTP_APIS_DEFAULT_SCHEME
  #if (MEMFAULT_HTTP_APIS_DEFAULT_PORT == 80)
    #define MEMFAULT_HTTP_APIS_DEFAULT_SCHEME "http"
  #else
    #define MEMFAULT_HTTP_APIS_DEFAULT_SCHEME "https"
  #endif
#endif

//
// Util Configuration Options
//

// Enables the use of a (512 bytes) lookup table for CRC16 computation to improve performance
//
// For extremely constrained environments where a small amount of data is being sent anyway the
// lookup table can be disabled to save ~500 bytes of flash space
#ifndef MEMFAULT_CRC16_LOOKUP_TABLE_ENABLE
#define MEMFAULT_CRC16_LOOKUP_TABLE_ENABLE 1
#endif

//
// Demo Configuration Options
//

#ifndef MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES
#define MEMFAULT_CLI_LOG_BUFFER_MAX_SIZE_BYTES (80)
#endif

#ifndef MEMFAULT_DEMO_CLI_USER_CHUNK_SIZE
// Note: Arbitrary default size for CLI command. Can be as small as 9 bytes.
#define MEMFAULT_DEMO_CLI_USER_CHUNK_SIZE 1024
#endif

//! The maximum length supported for a single CLI command
#ifndef MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE
#define MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE 64
#endif

//
// Custom Data Recording configuration options [EXPERIMENTAL]
//

//! Controls whether or not publishing Custom Data Recordings is enabled.
//! For severely constrained environments, this option can be disabled to save
//! a little flash/data space.
#ifndef MEMFAULT_CDR_ENABLE
#define MEMFAULT_CDR_ENABLE 0
#endif

//! Controls the maximum number of Custom Data Recording sources a project can register.
//! The overhead per allocation is 4 bytes (size of a pointer)
#ifndef MEMFAULT_CDR_MAX_DATA_SOURCES
#define MEMFAULT_CDR_MAX_DATA_SOURCES 4
#endif

//! Controls the maximum space budgeted for a Custom Data Recording header.
#ifndef MEMFAULT_CDR_MAX_ENCODED_METADATA_LEN
#define MEMFAULT_CDR_MAX_ENCODED_METADATA_LEN 128
#endif

//
// Port Configuration Options
//

//! Controls whether or not MCU reboot info is printed when
//! memfault_reboot_reason_get() is called
#ifndef MEMFAULT_ENABLE_REBOOT_DIAG_DUMP
#define MEMFAULT_ENABLE_REBOOT_DIAG_DUMP 1
#endif

//! Controls whether or not MCU reboot info is reset when
//! memfault_reboot_reason_get() is called
#ifndef MEMFAULT_REBOOT_REASON_CLEAR
#define MEMFAULT_REBOOT_REASON_CLEAR 1
#endif

//! Timeout to set Software Watchdog expiration for
#ifndef MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS
#define MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS 10
#endif

//! The maximum number of tasks which can be tracked by this subsystem at one
//! time. If your system has more tasks than this, the number will need to be
//! bumped in order for the stacks to be fully recovered
#ifndef MEMFAULT_PLATFORM_MAX_TRACKED_TASKS
#define MEMFAULT_PLATFORM_MAX_TRACKED_TASKS 16
#endif

//! The default amount of stack for each task to collect in bytes.  The larger
//! the size, the more stack frames Memfault will be able to unwind when the
//! coredump is uploaded.
#ifndef MEMFAULT_PLATFORM_TASK_STACK_SIZE_TO_COLLECT
#define MEMFAULT_PLATFORM_TASK_STACK_SIZE_TO_COLLECT 256
#endif

//! The default amount of stack to collect for the stack that was active leading up to a crash
#ifndef MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT
#define MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT 512
#endif

//
// Controls whether RAM or FLASH coredump storage port is picked up.
//

#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH 0
#endif

#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_RAM
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_RAM (!MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH)
#endif

#if MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH && MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_RAM
#error "Configuration error. Only one of _USE_FLASH or _USE_RAM can be set!"
#endif

//
// RAM backed coredump configuration options
//

//! When set to 1, end user must allocate and provide pointer to
//! coredump storage noinit region and define MEMFAULT_PLATFORM_COREDUMP_RAM_START_ADDR
//! in memfault_platform_config.h
#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_CUSTOM
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_CUSTOM 0
#endif

//! Controls the section name used for the noinit region a RAM backed coredump is saved to
//! Some vendor SDKs have pre-defined no-init regions in which case this can be overriden
#ifndef MEMFAULT_PLATFORM_COREDUMP_NOINIT_SECTION_NAME
#define MEMFAULT_PLATFORM_COREDUMP_NOINIT_SECTION_NAME ".noinit.mflt_coredump"
#endif

//! Controls the size of the RAM region allocated for coredump storage
#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 1024
#endif

//! The RAM port will by default collect the active stack at the time of crash.
//! Alternatively, an end user can define custom regions by implementing their own
//! version of memfault_platform_coredump_get_regions()
#ifndef MEMFAULT_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM
#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM 0
#endif

//! Set the capture size for FreeRTOS TCB structures in the coredump, when
//! memfault_freertos_ram_regions.c is used.
//!
//! Set this to 0 to collect the entire TCB structure.
//!
//! The default value captures the required structure fields in each TCB used
//! for RTOS Awareness by the Memfault backend, but truncates unused fields- for
//! example, if FreeRTOS is configured with configUSE_NEWLIB_REENTRANT, the TCBs
//! contain extra fields that are not needed for thread decode and take up space
//! in the coredump.
//!
//! See more details here: https://docs.memfault.com/docs/mcu/rtos-analysis
//!
//! And see ports/freertos/src/memfault_freertos_ram_regions.c for information
//! on the implementation
#ifndef MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE
  #define MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE 100
#endif

#ifdef __cplusplus
}
#endif
