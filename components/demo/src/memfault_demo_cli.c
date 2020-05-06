//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Memfault SDK

#include "memfault/demo/cli.h"

#include <stdlib.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/panics/reboot_tracking.h"

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

int memfault_demo_cli_cmd_post_core(int argc, char *argv[]) {
  MEMFAULT_LOG_INFO("Posting Memfault Data...");
  sMfltHttpClient *http_client = memfault_http_client_create();
  if (!http_client) {
    MEMFAULT_LOG_ERROR("Failed to create HTTP client");
    return MemfaultInternalReturnCode_Error;
  }
  const eMfltPostDataStatus rv =
      (eMfltPostDataStatus)memfault_http_client_post_data(http_client);
  if (rv == kMfltPostDataStatus_NoDataFound) {
    MEMFAULT_LOG_INFO("No new data found");
  } else {
    MEMFAULT_LOG_INFO("Result: %d", rv);
  }
  const uint32_t timeout_ms = 30 * 1000;
  memfault_http_client_wait_until_requests_completed(http_client, timeout_ms);
  memfault_http_client_destroy(http_client);
  return rv;
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

int memfault_demo_cli_cmd_get_device_info(int argc, char *argv[]) {
  struct MemfaultDeviceInfo info = {0};
  memfault_platform_get_device_info(&info);
  MEMFAULT_LOG_INFO("S/N: %s", info.device_serial ? info.device_serial : "<NULL>");
  MEMFAULT_LOG_INFO("SW type: %s", info.software_type ? info.software_type : "<NULL>");
  MEMFAULT_LOG_INFO("SW version: %s", info.software_version ? info.software_version : "<NULL>");
  MEMFAULT_LOG_INFO("HW version: %s", info.hardware_version ? info.hardware_version : "<NULL>");
  return 0;
}

int memfault_demo_cli_cmd_system_reboot(int argc, char *argv[]) {
  void *pc;
  MEMFAULT_GET_PC(pc);
  void *lr;
  MEMFAULT_GET_LR(lr);
  sMfltRebootTrackingRegInfo reg_info = {
    .pc = (uint32_t)pc,
    .lr = (uint32_t)lr,
  };

  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_UserReset, &reg_info);
  memfault_platform_reboot();
  return 0;
}
