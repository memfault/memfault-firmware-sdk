//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands which require integration of the "panic" component.

#include <inttypes.h>
#include <stdlib.h>

#include "memfault/core/arch.h"
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/demo/cli.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault_demo_cli_aux_private.h"

// Allow opting out of the cassert demo. Some platforms may not have a libc
// assert implementation.
#if !defined(MEMFAULT_DEMO_DISABLE_CASSERT)
  #include <assert.h>
  #define MEMFAULT_DEMO_DISABLE_CASSERT 0
#endif

MEMFAULT_NO_OPT
static void do_some_work_base(char *argv[]) {
  // An assert that is guaranteed to fail. We perform
  // the check against argv so that the compiler can't
  // perform any optimizations
  MEMFAULT_ASSERT((uint32_t)(uintptr_t)argv == 0xdeadbeef);
}

MEMFAULT_NO_OPT
static void do_some_work1(char *argv[]) {
  do_some_work_base(argv);
}

MEMFAULT_NO_OPT
static void do_some_work2(char *argv[]) {
  do_some_work1(argv);
}

MEMFAULT_NO_OPT
static void do_some_work3(char *argv[]) {
  do_some_work2(argv);
}

MEMFAULT_NO_OPT
static void do_some_work4(char *argv[]) {
  do_some_work3(argv);
}

MEMFAULT_NO_OPT
static void do_some_work5(char *argv[]) {
  do_some_work4(argv);
}

int memfault_demo_cli_cmd_crash(int argc, char *argv[]) {
  int crash_type = 0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  switch (crash_type) {
    case 0:
      MEMFAULT_ASSERT(0);
      break;

    case 1:
      g_bad_func_call();
      break;

    case 2: {
      uint64_t *buf = g_memfault_unaligned_buffer;
      *buf = 0xbadcafe0000;
    } break;

    case 3:
      do_some_work5(argv);
      break;

    case 4:
      MEMFAULT_SOFTWARE_WATCHDOG();
      break;

    default:
      // this should only ever be reached if crash_type is invalid
      MEMFAULT_LOG_ERROR("Usage: \"crash\" or \"crash <n>\" where n is 0..4");
      return -1;
  }

  // Should be unreachable. If we get here, trigger an assert and record the crash_type which
  // failed to trigger a crash
  MEMFAULT_ASSERT_RECORD((uint32_t)crash_type);
  return -1;
}

int memfault_demo_cli_cmd_get_core(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  size_t total_size = 0;
  if (!memfault_coredump_has_valid_coredump(&total_size)) {
    MEMFAULT_LOG_INFO("No coredump present!");
    return 0;
  }
  MEMFAULT_LOG_INFO("Has coredump with size: %d", (int)total_size);
  return 0;
}

int memfault_demo_cli_cmd_clear_core(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  MEMFAULT_LOG_INFO("Invalidating coredump");
  memfault_platform_coredump_storage_clear();
  return 0;
}

int memfault_demo_cli_cmd_coredump_size(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  size_t total_size = 0;
  size_t capacity = 0;
  memfault_coredump_size_and_storage_capacity(&total_size, &capacity);
  MEMFAULT_LOG_INFO("Coredump size: %d, capacity: %d", (int)total_size, (int)capacity);
  return 0;
}

int memfault_demo_cli_cmd_assert(int argc, char *argv[]) {
  // permit running with a user-provided "extra" value for testing that path
  if (argc > 1) {
    MEMFAULT_ASSERT_RECORD(atoi(argv[1]));
  } else {
    MEMFAULT_ASSERT(0);
  }

  return -1;
}

int memfault_demo_cli_cmd_cassert(int argc, char *argv[]) {
  (void)argc, (void)argv;

#if MEMFAULT_DEMO_DISABLE_CASSERT
  MEMFAULT_LOG_ERROR("C assert demo disabled");
#else
  assert(0);
#endif

  return -1;
}

#if MEMFAULT_COMPILER_ARM_CORTEX_M

int memfault_demo_cli_cmd_hardfault(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  memfault_arch_disable_configurable_faults();

  uint64_t *buf = g_memfault_unaligned_buffer;
  *buf = 0xdeadbeef0000;

  return -1;
}

int memfault_demo_cli_cmd_memmanage(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  // Per "Relation of the MPU to the system memory map" in ARMv7-M reference manual:
  //
  // "The MPU is restricted in how it can change the default memory map attributes associated with
  //  System space, that is, for addresses 0xE0000000 and higher. System space is always marked as
  //  XN, Execute Never."
  //
  // So we can trip a MemManage exception by simply attempting to execute any addresss >=
  // 0xE000.0000
  void (*bad_func)(void) = (void (*)(void))0xEEEEDEAD;
  bad_func();

  // We should never get here -- platforms MemManage or HardFault handler should be tripped
  return -1;
}

int memfault_demo_cli_cmd_busfault(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  void (*unaligned_func)(void) = (void (*)(void))0x50000001;
  unaligned_func();

  // We should never get here -- platforms BusFault or HardFault handler should be tripped
  // with a precise error due to unaligned execution
  return -1;
}

int memfault_demo_cli_cmd_usagefault(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  uint64_t *buf = g_memfault_unaligned_buffer;
  *buf = 0xbadcafe0000;

  // We should never get here -- platforms UsageFault or HardFault handler should be tripped due to
  // unaligned access
  return -1;
}

int memfault_demo_cli_loadaddr(int argc, char *argv[]) {
  if (argc < 2) {
    MEMFAULT_LOG_ERROR("Usage: loadaddr <addr>");
    return -1;
  }
  uint32_t addr = (uint32_t)strtoul(argv[1], NULL, 0);
  uint32_t val = *(uint32_t *)addr;

  MEMFAULT_LOG_INFO("Read 0x%08" PRIx32 " from 0x%08" PRIx32, val, (uint32_t)(uintptr_t)addr);
  return 0;
}

#endif  // MEMFAULT_COMPILER_ARM_CORTEX_M

#if MEMFAULT_COMPILER_ARM_V7_A_R

int memfault_demo_cli_cmd_dataabort(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  // try to write to a read-only address
  volatile int explode = *(int *)0xFFFFFFFF;

  return explode | 1;
}

int memfault_demo_cli_cmd_prefetchabort(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char *argv[]) {
  // We can trip a PrefetchAbort exception by simply attempting to execute any addresss >=
  // 0xE000.0000
  void (*bad_func)(void) = (void (*)(void))0xEEEEDEAD;
  bad_func();

  // We should never get here -- platforms PrefetchAbort handler should be tripped
  return -1;
}

#endif  // MEMFAULT_COMPILER_ARM_V7_A_R
