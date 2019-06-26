#pragma once

//! @file
//!
//! @brief CLI console commands for Memfault demo apps

//! Command to crash the device, for example, to trigger a coredump to be captured.
//! It takes one number argument, which is the crash type:
//! - 0: crash by MEMFAULT_ASSERT(0)
//! - 1: crash by jumping to 0xbadcafe
//! - 2: crash by unaligned memory store
int memfault_demo_cli_cmd_crash(int argc, char *argv[]);

//! Command to get whether a coredump is currently stored and how large it is.
//! It takes no arguments.
int memfault_demo_cli_cmd_get_core(int argc, char *argv[]);

//! Command to post a coredump using the http client.
//! It takes no arguments.
int memfault_demo_cli_cmd_post_core(int argc, char *argv[]);

//! Command to delete a coredump.
//! It takes no arguments.
int memfault_demo_cli_cmd_delete_core(int argc, char *argv[]);

//! Command to print device info, as obtained through memfault_platform_get_device_info().
//! It takes no arguments.
int memfault_demo_cli_cmd_get_device_info(int argc, char *argv[]);
