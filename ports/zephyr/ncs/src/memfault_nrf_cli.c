//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

#include "memfault/core/debug_log.h"
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

#if CONFIG_MEMFAULT_HTTP_ENABLE
static int prv_mflt_get_latest_url(const struct shell *shell, size_t argc, char **argv) {
  char *url = NULL;
  int rv = memfault_zephyr_port_get_download_url(&url);
  if (rv <= 0) {
    MEMFAULT_LOG_ERROR("Unable to fetch OTA url, rv=%d", rv);
    return rv;
  }

  printk("Download URL: '%s'\n", url);

  rv = memfault_zephyr_port_release_download_url(&url);

  return rv;
}
#endif // CONFIG_MEMFAULT_HTTP_ENABLE

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_memfault_nrf_cmds,
    SHELL_CMD(fota, NULL, "Perform a FOTA using Memfault client",
              prv_mflt_fota),
#if CONFIG_MEMFAULT_HTTP_ENABLE
    SHELL_CMD(get_latest_url, NULL, "Get the latest URL for the latest FOTA",
              prv_mflt_get_latest_url),
#endif
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(mflt_nrf, &sub_memfault_nrf_cmds,
                   "Memfault nRF Connect SDK Test Commands", NULL);
