//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

#include "memfault/core/debug_log.h"
#include "memfault/nrfconnect_port/coap.h"
#include "memfault/nrfconnect_port/fota.h"
#include "memfault/ports/zephyr/http.h"
// clang-format on

static int prv_mflt_fota(const struct shell *shell, size_t argc, char **argv) {
#if CONFIG_MEMFAULT_FOTA_CLI_CMD
  MEMFAULT_LOG_INFO("Checking for FOTA");
  const int rv = memfault_fota_start();
  if (rv == 0) {
    MEMFAULT_LOG_INFO("FW is up to date!");
  }
  return rv;
#else
  shell_print(shell, "CONFIG_MEMFAULT_FOTA_CLI_CMD not enabled");
  return -1;
#endif
}

static int prv_mflt_get_latest_url(const struct shell *shell, size_t argc, char **argv) {
#if CONFIG_MEMFAULT_HTTP_ENABLE || CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  char *protocol = (argc >= 2) ? argv[1] : "https";
#else
  MEMFAULT_LOG_ERROR("Either HTTPS or nRF Cloud CoAP required");
#endif

  char *url = NULL;
  int rv = -1;

#if CONFIG_MEMFAULT_HTTP_ENABLE
  if (strcmp(protocol, "https") == 0) {
    MEMFAULT_LOG_INFO("Checking for FOTA over HTTPS!");
    rv = memfault_zephyr_port_get_download_url(&url);
  }
#endif

#if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  if (strcmp(protocol, "coap") == 0) {
    MEMFAULT_LOG_INFO("Checking for FOTA over COAP!");
    rv = memfault_zephyr_port_coap_get_download_url(&url);
  }
#endif

  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Unable to fetch OTA url, rv=%d", rv);
    return rv;
  }

  if (rv == 0) {
    MEMFAULT_LOG_INFO("FOTA up to date!");
    return rv;
  }

  printk("Download URL: '%s'\n", url);

#if CONFIG_MEMFAULT_HTTP_ENABLE
  if (strcmp(protocol, "https") == 0) {
    memfault_zephyr_port_release_download_url(&url);
  }
#endif

#if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  if (strcmp(protocol, "coap") == 0) {
    memfault_zephyr_port_coap_release_download_url(&url);
  }
#endif

  return rv;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_memfault_nrf_cmds,
                               SHELL_CMD(fota, NULL, "Perform a FOTA using Memfault client",
                                         prv_mflt_fota),
                               SHELL_CMD(get_latest_url, NULL,
                                         "Get the latest URL for the latest FOTA",
                                         prv_mflt_get_latest_url),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mflt_nrf, &sub_memfault_nrf_cmds, "Memfault nRF Connect SDK Test Commands",
                   NULL);
