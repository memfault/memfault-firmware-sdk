#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief CLI console commands for Memfault demo apps
//!
//! The first element in argv is expected to be the command name that was invoked.
//! The elements following after that are expected to be the arguments to the command.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/core/compiler.h"

//! Command to crash the device, for example, to trigger a coredump to be captured.
//! It takes one number argument, which is the crash type:
//! - 0: crash by MEMFAULT_ASSERT(0)
//! - 1: crash by jumping to 0xbadcafe
//! - 2: crash by unaligned memory store
int memfault_demo_cli_cmd_crash(int argc, char *argv[]);

#if MEMFAULT_COMPILER_ARM_CORTEX_M

//! Command which will generate a HardFault
int memfault_demo_cli_cmd_hardfault(int argc, char *argv[]);

//! Command which will generate a BusFault on Cortex-M hardware
int memfault_demo_cli_cmd_busfault(int argc, char *argv[]);

//! Command which will generate a Memory Management fault on Cortex-M hardware
int memfault_demo_cli_cmd_memmanage(int argc, char *argv[]);

//! Command which will generate a UsageFault on Cortex-M hardware
int memfault_demo_cli_cmd_usagefault(int argc, char *argv[]);

//! Read a 32-bit memory address and print the value. Can be used to test
//! specific faults due to protected regions
int memfault_demo_cli_loadaddr(int argc, char *argv[]);

#endif  // MEMFAULT_COMPILER_ARM_CORTEX_M

#if MEMFAULT_COMPILER_ARM_V7_A_R
//! Trigger a data abort on an ARMv7-A/R chip
int memfault_demo_cli_cmd_dataabort(int argc, char *argv[]);

//! Trigger a prefetch abort on an ARMv7-A/R chip
int memfault_demo_cli_cmd_prefetchabort(int argc, char *argv[]);

#endif  // MEMFAULT_COMPILER_ARM_V7_A_R

//! Command which will generate an assert
int memfault_demo_cli_cmd_assert(int argc, char *argv[]);

//! Command which will generate a libc assert()
int memfault_demo_cli_cmd_cassert(int argc, char *argv[]);

//! Command to exercise the MEMFAULT_TRACE_EVENT API, capturing a
//! Trace Event with the error reason set to "MemfaultDemoCli_Error".
int memfault_demo_cli_cmd_trace_event_capture(int argc, char *argv[]);

//! Command to insert test logs into the RAM log buffer. One log for each log level
//! is written. The command takes no arguments.
//! @note By default, the minimum save level is >= Info. Use the
//! memfault_log_set_min_save_level() API to control this.
int memfault_demo_cli_cmd_test_log(int argc, char *argv[]);

//! Command to trigger "freeze" the current contents of the RAM log buffer and
//! allow them to be collected through the data transport (see memfault_demo_drain_chunk_data()).
//! It takes no arguments.
int memfault_demo_cli_cmd_trigger_logs(int argc, char *argv[]);

//! Command to get whether a coredump is currently stored and how large it is.
//! It takes no arguments.
int memfault_demo_cli_cmd_get_core(int argc, char *argv[]);

//! Command to post a coredump using the http client.
//! It takes no arguments.
int memfault_demo_cli_cmd_post_core(int argc, char *argv[]);

//! Command to clear a coredump.
//! It takes no arguments.
int memfault_demo_cli_cmd_clear_core(int argc, char *argv[]);

//! Print coredump size and storage capacity
int memfault_demo_cli_cmd_coredump_size(int argc, char *argv[]);

//! Command to print device info, as obtained through memfault_platform_get_device_info().
//! It takes no arguments.
int memfault_demo_cli_cmd_get_device_info(int argc, char *argv[]);

//! Reboots the system
//!
//! This command takes no arguments and demonstrates how to use the reboot tracking module to
//! track the occurrence of intentional reboots.
int memfault_demo_cli_cmd_system_reboot(int argc, char *argv[]);

//! Drains _all_ queued up chunks by calling user_transport_send_chunk_data
//!
//! @note user_transport_send_chunk_data is defined as a weak function so it can be overriden.
//! The default implementation is a no-op.
//! @note When "memfault install_chunk_handler" has been run, this can be used as a way to post
//! chunks to the Memfault cloud directly from GDB. See https://mflt.io/posting-chunks-with-gdb
//! for more details
int memfault_demo_drain_chunk_data(int argc, char *argv[]);
void user_transport_send_chunk_data(void *chunk_data, size_t chunk_data_len);

//! Output base64 encoded chunks. Chunks can be uploaded via the Memfault CLI or
//! manually via the Chunks Debug in the UI.
int memfault_demo_cli_cmd_export(int argc, char *argv[]);

//! Print current heartbeat metrics
int memfault_demo_cli_cmd_heartbeat_dump(int argc, char *argv[]);

//! Trigger a heartbeat
int memfault_demo_cli_cmd_heartbeat(int argc, char *argv[]);

//! Run a self test
//!
//! This command triggers a test which exercises different subsystems
int memfault_demo_cli_cmd_self_test(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
