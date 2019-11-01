//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Memfault SDK

#include "memfault/demo/cli.h"

#include <stdlib.h>

#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"
#include "memfault/panics/assert.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/platform/coredump.h"

// Defined in memfault_demo_cli_aux.c
extern void *g_memfault_unaligned_buffer;
extern void (*g_bad_func_call)(void);

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
  }

  // this should only ever be reached if crash_type is invalid
  MEMFAULT_LOG_ERROR("Usage: \"crash\" or \"crash <n>\" where n is 0..2");
  return -1;
}

int memfault_demo_cli_cmd_post_core(int argc, char *argv[]) {
  MEMFAULT_LOG_INFO("Posting coredump...");
  sMfltHttpClient *http_client = memfault_http_client_create();
  if (!http_client) {
    MEMFAULT_LOG_ERROR("Failed to create HTTP client");
    return MemfaultInternalReturnCode_Error;
  }
  const eMfltPostDataStatus rv = memfault_http_client_post_data(http_client);
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

int memfault_demo_cli_cmd_delete_core(int argc, char *argv[]) {
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
