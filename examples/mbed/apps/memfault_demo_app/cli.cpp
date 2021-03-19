//! @file
//! @brief
//! This demonstration CLI provides a way to interact with the Mbed
//! platform reference implementation by providing commands to crash
//! and get/delete/print the stored coredump.

#include "cli.h"

#include "mbed.h"
#include "memfault/core/debug_log.h"
#include "memfault/demo/cli.h"
#include "memfault/demo/shell.h"

static int prv_putchar_shim(char c) {
  return putchar((int)c);
}

void cli_init(void) {
  const sMemfaultShellImpl s_shell_impl = {
    .send_char = prv_putchar_shim
  };
  memfault_demo_shell_boot(&s_shell_impl);
}

MEMFAULT_NORETURN void cli_forever(void) {
  while (true) {
    int c = getchar();
    if (c != EOF) {
      memfault_demo_shell_receive_char((char)c);
    }
  }

  MEMFAULT_UNREACHABLE;
}
