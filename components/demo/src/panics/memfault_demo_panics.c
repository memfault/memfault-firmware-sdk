//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands which require integration of the "panic" component.

#include "memfault/demo/cli.h"

#include <stdlib.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"

// Defined in memfault_demo_cli_aux.c
extern void *g_memfault_unaligned_buffer;
extern void (*g_bad_func_call)(void);

MEMFAULT_NO_OPT
void do_some_work_base(char *argv[]) {
  // An assert that is guaranteed to fail. We perform
  // the check against argv so that the compiler can't
  // perform any optimizations
  MEMFAULT_ASSERT((uint32_t)argv == 0xdeadbeef);
}

MEMFAULT_NO_OPT
void do_some_work1(char *argv[]) {
  do_some_work_base(argv);
}

MEMFAULT_NO_OPT
void do_some_work2(char *argv[]) {
  do_some_work1(argv);
}

MEMFAULT_NO_OPT
void do_some_work3(char *argv[]) {
  do_some_work2(argv);
}

MEMFAULT_NO_OPT
void do_some_work4(char *argv[]) {
  do_some_work3(argv);
}

MEMFAULT_NO_OPT
void do_some_work5(char *argv[]) {
  do_some_work4(argv);
}

int memfault_demo_cli_cmd_crash(int argc, char *argv[]) {
  int crash_type = 0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  if (crash_type == 0) {
    MEMFAULT_ASSERT(0);
  } else if (crash_type == 1) {
    g_bad_func_call();
  } else if (crash_type == 2) {
    uint64_t *buf = g_memfault_unaligned_buffer;
    *buf = 0xbadcafe0000;
  } else if (crash_type == 3) {
    do_some_work5(argv);
  } else {
    // this should only ever be reached if crash_type is invalid
    MEMFAULT_LOG_ERROR("Usage: \"crash\" or \"crash <n>\" where n is 0..3");
    return -1;
  }

  // Should be unreachable. If we get here, trigger an assert and record the crash_type which
  // failed to trigger a crash
  MEMFAULT_ASSERT_RECORD(crash_type);
  return -1;
}

int memfault_demo_cli_cmd_get_core(int argc, char *argv[]) {
  size_t total_size = 0;
  if (!memfault_coredump_has_valid_coredump(&total_size)) {
    MEMFAULT_LOG_INFO("No coredump present!");
    return 0;
  }
  MEMFAULT_LOG_INFO("Has coredump with size: %u", total_size);
  return 0;
}

int memfault_demo_cli_cmd_clear_core(int argc, char *argv[]) {
  MEMFAULT_LOG_INFO("Invalidating coredump");
  memfault_platform_coredump_storage_clear();
  return 0;
}
