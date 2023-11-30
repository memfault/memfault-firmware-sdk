#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Command definitions for the minimal shell/console implementation.

#include <stddef.h>

#include "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemfaultShellCommand {
  const char *command;
  int (*handler)(int argc, char *argv[]);
  const char *help;
} sMemfaultShellCommand;

extern const sMemfaultShellCommand *const g_memfault_shell_commands;
extern const size_t g_memfault_num_shell_commands;

int memfault_shell_help_handler(int argc, char *argv[]);

#if defined(MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS)
//! Set extension command list.
//!
//! @arg commands: Pointer to the command list to set
//! @arg num_commands: Number of commands in the list.
void memfault_shell_command_set_extensions(const sMemfaultShellCommand *const commands,
                                           size_t num_commands);
#endif

#ifdef __cplusplus
}
#endif
