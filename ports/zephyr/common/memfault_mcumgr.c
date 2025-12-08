//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! MCUmgr group handler for Memfault device information and project key access

#include <string.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/mcumgr_memfault.h"

LOG_MODULE_REGISTER(mcumgr_memfault_grp, CONFIG_MEMFAULT_LOG_LEVEL);

/**
 * Command handler for reading device information.
 * Returns device_serial, hardware_version, software_type, and current_version
 * from memfault_platform_get_device_info()
 */
static int memfault_mgmt_device_info(struct smp_streamer *ctxt) {
  zcbor_state_t *zse = ctxt->writer->zs;
  sMemfaultDeviceInfo device_info = { 0 };
  bool ok;

  LOG_DBG("Memfault device info command called");

  // Get device information from the Memfault platform API
  memfault_platform_get_device_info(&device_info);

  // Encode the response as CBOR
  ok =
    zcbor_tstr_put_lit(zse, "device_serial") &&
    zcbor_tstr_put_term(zse, device_info.device_serial ? device_info.device_serial : "",
                        strlen(device_info.device_serial ? device_info.device_serial : "")) &&
    zcbor_tstr_put_lit(zse, "hardware_version") &&
    zcbor_tstr_put_term(zse, device_info.hardware_version ? device_info.hardware_version : "",
                        strlen(device_info.hardware_version ? device_info.hardware_version : "")) &&
    zcbor_tstr_put_lit(zse, "software_type") &&
    zcbor_tstr_put_term(zse, device_info.software_type ? device_info.software_type : "",
                        strlen(device_info.software_type ? device_info.software_type : "")) &&
    zcbor_tstr_put_lit(zse, "current_version") &&
    zcbor_tstr_put_term(zse, device_info.software_version ? device_info.software_version : "",
                        strlen(device_info.software_version ? device_info.software_version : ""));

  return (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
}

/**
 * Command handler for reading the Memfault project key.
 * Returns the project key string from Kconfig.
 */
static int memfault_mgmt_project_key(struct smp_streamer *ctxt) {
  zcbor_state_t *zse = ctxt->writer->zs;
  bool ok;

  LOG_DBG("Memfault project key command called");

#ifdef CONFIG_MEMFAULT_NCS_PROJECT_KEY
  const char *project_key = CONFIG_MEMFAULT_NCS_PROJECT_KEY;
#else
  const char *project_key = CONFIG_MEMFAULT_PROJECT_KEY;
#endif

  // Check if project key is configured
  if (!project_key || strlen(project_key) == 0) {
    ok = smp_add_cmd_err(zse, MGMT_GROUP_ID_MEMFAULT, MEMFAULT_MGMT_ERR_NO_PROJECT_KEY);
    return (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
  }

  // Encode the response as CBOR
  ok = zcbor_tstr_put_lit(zse, "project_key") &&
       zcbor_tstr_put_term(zse, project_key, strlen(project_key));

  return (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);
}

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/**
 * Translate SMP version 2 error codes to legacy MCUmgr error codes.
 * Only included if support for the original protocol is enabled.
 */
static int memfault_mgmt_translate_error_code(uint16_t err) {
  int rc;

  switch (err) {
    case MEMFAULT_MGMT_ERR_NO_PROJECT_KEY:
      rc = MGMT_ERR_ENOENT;
      break;

    case MEMFAULT_MGMT_ERR_UNKNOWN:
    default:
      rc = MGMT_ERR_EUNKNOWN;
  }

  return rc;
}
#endif

static const struct mgmt_handler memfault_mgmt_handlers[] = {
  [MEMFAULT_MGMT_ID_DEVICE_INFO] = {
    .mh_read = memfault_mgmt_device_info,
    .mh_write = NULL,
  },
  [MEMFAULT_MGMT_ID_PROJECT_KEY] = {
    .mh_read = memfault_mgmt_project_key,
    .mh_write = NULL,
  },
};

static struct mgmt_group memfault_mgmt_group = {
  .mg_handlers = memfault_mgmt_handlers,
  .mg_handlers_count = ARRAY_SIZE(memfault_mgmt_handlers),
  .mg_group_id = MGMT_GROUP_ID_MEMFAULT,
#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
  .mg_translate_error = memfault_mgmt_translate_error_code,
#endif
};

static void memfault_mgmt_register_group(void) {
  // This function is called during system init before main() is invoked.
  // Register the group so that clients can call the function handlers.
  mgmt_register_group(&memfault_mgmt_group);
  LOG_INF("Memfault MCUmgr group registered (group ID: %d)", MGMT_GROUP_ID_MEMFAULT);
}

MCUMGR_HANDLER_DEFINE(memfault_mgmt, memfault_mgmt_register_group);
