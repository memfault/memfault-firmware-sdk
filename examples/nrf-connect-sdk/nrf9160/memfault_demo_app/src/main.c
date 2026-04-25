//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/printk.h)
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)
#include <string.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/thread_metrics.h"
#include "memfault/ports/zephyr/http.h"
#include "memfault/ports/ncs/version.h"
#include "memfault/ports/ncs/date_time_callback.h"
#include "memfault_demo_app.h"

#include <modem/modem_info.h>
#include <modem/lte_lc.h>

#if defined(CONFIG_NRF_CLOUD_COAP)
#include <date_time.h>
#include <net/nrf_cloud_coap.h>

static K_SEM_DEFINE(s_date_time_ready, 0, 1);

static void prv_date_time_evt_handler(const struct date_time_evt *evt) {
#if defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_DATETIME)
  // Forward event to Memfault's date/time callback handler, which will update
  // the system time used for timestamping events and coredumps
  memfault_zephyr_date_time_evt_handler(evt);
#endif

  // Only signal that date/time is ready on successful time obtain events
  if ((evt != NULL) && (evt->type != DATE_TIME_NOT_OBTAINED)) {
    k_sem_give(&s_date_time_ready);
  }
}
#endif

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

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)
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
#endif

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

#if defined(CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME)
  // requires AT modem interface to be up
  prv_init_device_info();
#endif

#if defined(CONFIG_MEMFAULT_HTTP_ENABLE) && \
  !defined(CONFIG_MEMFAULT_ROOT_CERT_INSTALL_ON_MODEM_LIB_INIT)
  // Note: We must provision certs prior to connecting to the LTE network
  // in order to use them
  err = memfault_zephyr_port_install_root_certs();
  if (err) {
    goto cleanup;
  }
#endif

#if defined(CONFIG_MEMFAULT_HTTP_SOCKET_DISPATCH)
  // Set the network interface name to use for Memfault HTTP uploads
  err = memfault_zephyr_port_http_set_interface_name("net0");
  if (err) {
    printk("Failed to set Memfault HTTP network interface name, err %d\n", err);
    goto cleanup;
  }
#endif

#if defined(CONFIG_NRF_CLOUD_COAP)
  // Register early so we don't miss the time-update event after LTE connects.
  // JWT authentication (used by nRF Cloud CoAP) requires a valid timestamp.
  date_time_register_handler(prv_date_time_evt_handler);
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

#if defined(CONFIG_NRF_CLOUD_COAP)
  // Wait for the date/time library to sync from the network (up to 10 minutes).
  // nrf_cloud_coap_connect() uses a JWT signed with the current time; connecting
  // before the clock is set will result in an authentication failure.
  printk("Waiting for date/time sync...\n");
  k_sem_take(&s_date_time_ready, K_MINUTES(10));
  if (!date_time_is_valid()) {
    printk("Warning: date/time not yet valid, CoAP connect may fail\n");
  }

  err = nrf_cloud_coap_init();
  if (err) {
    printk("Failed to initialize nRF Cloud CoAP client, err %d\n", err);
    goto cleanup;
  }

  err = nrf_cloud_coap_connect(NULL);
  if (err) {
    printk("Failed to connect to nRF Cloud via CoAP, err %d\n", err);
    goto cleanup;
  }
  printk("Connected to nRF Cloud via CoAP\n");
#endif

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
