//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
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
#include <string.h>

#include "memfault/core/build_info.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"
#include "memfault/nrfconnect_port/http.h"
#include "memfault/ports/ncs/version.h"
#include "memfault_demo_app.h"

// nRF Connect SDK < 1.3
#if (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR < 3)

#include <lte_lc.h>
#include <at_cmd.h>
#include <at_notif.h>
#include <modem_key_mgmt.h>
#include <net/bsdlib.h>
#include <modem_info.h>

// nRF Connect SDK < 1.9
#elif (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR < 9) && (NCS_PATCHLEVEL < 99)

#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_key_mgmt.h>
#include <modem/modem_info.h>

// nRF Connect SDK >= 1.9
#else
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>

#endif

#if (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR <= 2)
#include <net/bsdlib.h>
static int prv_init_modem_lib(void) {
  return bsdlib_init();
}
#elif (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR <= 4)
#include <modem/bsdlib.h>
static int prv_init_modem_lib(void) {
  return bsdlib_init();
}
#elif (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR <= 5)
#include <modem/nrf_modem_lib.h>
static int prv_init_modem_lib(void) {
  return nrf_modem_lib_init(NORMAL_MODE);
}
#elif !MEMFAULT_NCS_VERSION_GT(2, 3)
/* nRF Connect SDK >= 1.6 || <= 2.3 */
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

// A direct Memfault integration was added in 1.6
#if (NCS_VERSION_MAJOR == 1) && (NCS_VERSION_MINOR < 6)

// Note: Starting with nRF Connect SDK 1.6, there is a direct integration of Memfault
// and these dependencies are no longer needed!
// See https://mflt.io/nrf-connect-sdk-lib for more details

static char s_fw_version[16] = "1.0.0-dev";

#ifndef MEMFAULT_NCS_PROJECT_KEY
#define MEMFAULT_NCS_PROJECT_KEY "<YOUR PROJECT KEY HERE>"
#endif

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = MEMFAULT_NCS_PROJECT_KEY,
};

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  // platform specific version information
  *info = (sMemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .software_type = "nrf91ns-fw",
    .software_version = s_fw_version,
    .hardware_version = "nrf9160dk",
  };
}

// stub for function implemented in nRF Connect SDK >= 1.6
static int memfault_ncs_device_id_set(const char *device_id, size_t len) {
  return 0;
}

#endif

#if !MEMFAULT_NCS_VERSION_GT(1, 8)
static int query_modem(const char *cmd, char *buf, size_t buf_len) {
  enum at_cmd_state at_state;
  int ret = at_cmd_write(cmd, buf, buf_len, &at_state);
  if (ret != 0) {
    printk("at_cmd_write [%s] error: %d, at_state: %d\n", cmd, ret, at_state);
  }

  return ret;
}
#endif

static int prv_get_imei(char *buf, size_t buf_len) {
#if MEMFAULT_NCS_VERSION_GT(1, 8)
  // use the cached modem info to fetch the IMEI
  int err = modem_info_init();
  if (err != 0) {
    printk("Modem info Init error: %d\n", err);
  } else {
    modem_info_string_get(MODEM_INFO_IMEI, buf, buf_len);
  }
  return err;
#else
  // use an AT command to read IMEI
  return query_modem("AT+CGSN", buf, buf_len);
#endif
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

  // These libraries were removed in ncs 1.9
#if !MEMFAULT_NCS_VERSION_GT(1, 8)
  err = at_cmd_init();
  if (err) {
    printk("Failed to initialize AT commands, err %d\n", err);
    goto cleanup;
  }

  err = at_notif_init();
  if (err) {
    printk("Failed to initialize AT notifications, err %d\n", err);
    goto cleanup;
  }
#endif

  // requires AT modem interface to be up
  prv_init_device_info();

  // Note: We must provision certs prior to connecting to the LTE network
  // in order to use them
  err = memfault_nrfconnect_port_install_root_certs();
  if (err) {
    goto cleanup;
  }

  printk("Waiting for network...\n");
  err = lte_lc_init_and_connect();
  if (err) {
    printk("Failed to connect to the LTE network, err %d\n", err);
    goto cleanup;
  }
  printk("OK\n");

cleanup:
  return err;
}
