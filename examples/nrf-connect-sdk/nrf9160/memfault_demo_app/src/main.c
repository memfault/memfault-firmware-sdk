//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Entry point to Memfault Radio. In this file you will find:
//!  1. Implementation for memfault_platform_get_device_info()
//!  2. g_mflt_http_client_config dependency which needs to be filled in with your Project Key
//!  3. A call to install the Root Certs used by Memfault on the nRF91 modem
//!     (memfault_nrfconnect_port_install_root_certs())
//!  4. Start of the LTE modem & AT interface

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/printk.h)
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)
#include <string.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/thread_metrics.h"
#include "memfault/nrfconnect_port/http.h"
#include "memfault/ports/ncs/version.h"
#include "memfault_demo_app.h"

#include <modem/modem_info.h>
#include <modem/lte_lc.h>

MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
{
  .thread_name = "idle",
  .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
},
{
  .thread_name = "sysworkq",
  .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_sysworkq_pct_max),
},
{
  .thread_name = "mflt_upload",
  .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_mflt_upload_pct_max),
},
{
  .thread_name = "🐶 wdt",
  .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_wdt_pct_max),
},
{
  .thread_name = "shell_uart",
  .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_shell_uart_pct_max),
}
);

#if !MEMFAULT_NCS_VERSION_GT(2, 3)
/* nRF Connect SDK >= 1.9.2 || <= 2.3 */
#include "memfault_ncs.h"
#include <modem/nrf_modem_lib.h>
static int prv_init_modem_lib(void) {
  return nrf_modem_lib_init(NORMAL_MODE);
}
#else /* nRF Connect SDK >= 2.4 */
#include "memfault_ncs.h"
#include <modem/nrf_modem_lib.h>
static int prv_init_modem_lib(void) {
  return nrf_modem_lib_init();
}
#endif

#if CONFIG_DFU_TARGET_MCUBOOT
#include MEMFAULT_ZEPHYR_INCLUDE(dfu/mcuboot.h)
#endif
// clang-format on

// Since the example app manages when the modem starts/stops, we manually configure the device
// serial even when using nRF Connect SDKs including the Memfault integration (by using
// CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)
#define IMEI_LEN 15
static char s_device_serial[IMEI_LEN + 1 /* '\0' */] = "unknown";

static int prv_get_imei(char *buf, size_t buf_len) {
  // use the cached modem info to fetch the IMEI
  int err = modem_info_init();
  if (err != 0) {
    printk("Modem info Init error: %d\n", err);
  } else {
    modem_info_string_get(MODEM_INFO_IMEI, buf, buf_len);
  }
  return err;
}

static void prv_init_device_info(void) {
  // we'll use the IMEI as the device serial
  char modem_info[MODEM_INFO_MAX_RESPONSE_SIZE];

  int ret = prv_get_imei(modem_info, sizeof(modem_info));
  if (ret != 0) {
    printk("Failed to get IMEI\n\r");
  } else {
    modem_info[IMEI_LEN] = '\0';
    strcpy(s_device_serial, modem_info);
    printk("IMEI: %s\n", s_device_serial);
  }

  // register the device id with memfault port so it is used for reporting
  memfault_ncs_device_id_set(s_device_serial, IMEI_LEN);
}

int main(void) {
  printk("Memfault Demo App Started!\n");

  memfault_demo_app_watchdog_boot();

#if CONFIG_DFU_TARGET_MCUBOOT
  if (!boot_is_img_confirmed()) {
    // Mark the ota image as installed so we don't revert
    printk("Confirming OTA update\n");
    boot_write_img_confirmed();
  }
#endif

  int err = prv_init_modem_lib();
  if (err) {
    printk("Failed to initialize modem!");
    goto cleanup;
  }

  // requires AT modem interface to be up
  prv_init_device_info();

  // Note: We must provision certs prior to connecting to the LTE network
  // in order to use them
  err = memfault_nrfconnect_port_install_root_certs();
  if (err) {
    goto cleanup;
  }

#if defined(CONFIG_MEMFAULT_HTTP_SOCKET_DISPATCH)
  // Set the network interface name to use for Memfault HTTP uploads
  err = memfault_zephyr_port_http_set_interface_name("net0");
  if (err) {
    printk("Failed to set Memfault HTTP network interface name, err %d\n", err);
    goto cleanup;
  }
#endif

  printk("Waiting for network...\n");
  // lte_lc_init_and_connect is deprecated in NCS 2.6
  err =
#if MEMFAULT_NCS_VERSION_GT(2, 5)
    lte_lc_connect();
#else
    lte_lc_init_and_connect();
#endif

  if (err) {
    printk("Failed to connect to the LTE network, err %d\n", err);
    goto cleanup;
  }
  printk("OK\n");

cleanup:
  return err;
}

static int prv_reboot_custom(const struct shell *shell, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(shell, "Usage: app reboot_custom <expected|unexpected>");
    return -1;
  }
  if (strcmp(argv[1], "expected") == 0) {
    MEMFAULT_REBOOT_MARK_RESET_IMMINENT_CUSTOM(CustomExpectedReboot);
  } else if (strcmp(argv[1], "unexpected") == 0) {
    MEMFAULT_REBOOT_MARK_RESET_IMMINENT_CUSTOM(CustomUnexpectedReboot);
  } else {
    shell_print(shell, "Invalid argument: %s", argv[1]);
    return -1;
  }

  shell_print(shell, "Rebooting with custom reason...");

  // Allow the shell output buffer to be flushed before the reboot
  k_sleep(K_MSEC(100));
  memfault_platform_reboot();
  return 0;  // should be unreachable
}

#if defined(CONFIG_MEMFAULT_HTTP_SOCKET_DISPATCH)
static int prv_set_net_iface(const struct shell *shell, size_t argc, char **argv) {
  if (argc != 2) {
    shell_print(shell, "Usage: app set_net_iface <if_name>");
    return -1;
  }
  const char *if_name = argv[1];
  int err = memfault_zephyr_port_http_set_interface_name(if_name);
  if (err) {
    shell_print(shell, "Failed to set Memfault HTTP network interface name, err %d", err);
    return err;
  }
  shell_print(shell, "Set Memfault HTTP network interface name to: %s", if_name);
  return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
  sub_app_cmds, SHELL_CMD(reboot_custom, NULL, "test custom reboot", prv_reboot_custom),
#if defined(CONFIG_MEMFAULT_HTTP_SOCKET_DISPATCH)
  SHELL_CMD(set_net_iface, NULL, "set net iface name", prv_set_net_iface),
#endif
  SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(app, &sub_app_cmds, "Memfault Demo App Test Commands", NULL);
