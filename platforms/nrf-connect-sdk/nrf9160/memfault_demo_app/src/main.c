//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Entry point to Memfault Radio. In this file you will find:
//!  1. Implementation for memfault_platform_get_device_info()
//!    - The firmware version uses the Memfault integration with the GNU Build ID
//!      (https://mflt.io/gnu-build-id) to guarantee dev builds always have a unique
//!      version
//!    - Looks up the IMEI and uses that as the device_serial
//!  2. g_mflt_http_client_config dependency which needs to be filled in with your Project API Key
//!  3. A call to install the Root Certs used by Memfault on the nRF91 modem
//!     (memfault_nrfconnect_port_install_root_certs())
//!  4. Start of the LTE modem & AT interface

#include <string.h>

#include <zephyr.h>
#include <sys/printk.h>

#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_key_mgmt.h>
#include <modem/bsdlib.h>

#include "memfault/core/build_info.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"
#include "memfault/nrfconnect_port/http.h"

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = "<YOUR API KEY HERE>",
};

static char s_fw_version[16]="1.0.0+";

#define IMEI_LEN 15
static char s_device_serial[IMEI_LEN + 1 /* '\0' */];

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  static bool s_init = false;

  if (!s_init) {
    const size_t version_len = strlen(s_fw_version);
    // We will use 6 characters of the build id to make our versions unique and
    // identifiable between releases
    const size_t build_id_chars = 6 + 1 /* '\0' */;

    const size_t build_id_num_chars =
        MEMFAULT_MIN(build_id_chars, sizeof(s_fw_version) - version_len - 1);

    memfault_build_id_get_string(&s_fw_version[version_len], build_id_num_chars);
    s_init = true;
  }

  // platform specific version information
  *info = (sMemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .software_type = "nrf91ns-fw",
    .software_version = s_fw_version,
    .hardware_version = "proto",
  };
}

static int query_modem(const char *cmd, char *buf, size_t buf_len) {
  enum at_cmd_state at_state;
  int ret = at_cmd_write(cmd, buf, buf_len, &at_state);

  if (ret != 0) {
    printk("at_cmd_write [%s] error:%d, at_state: %d",
            cmd, ret, at_state);
  }

  return ret;
}

static void prv_init_device_info(void) {
  // we'll use the IMEI as the device serial
  char imei_buf[IMEI_LEN + 2 /* for \r\n */ + 1 /* \0 */];
  if (query_modem("AT+CGSN", imei_buf, sizeof(imei_buf)) != 0) {
    strcat(s_device_serial, "Unknown");
    return;
  }

  imei_buf[IMEI_LEN] = '\0';
  strcat(s_device_serial, imei_buf);

  printk("Device Serial: %s\n", s_device_serial);
}

void main(void) {
  printk("Memfault Demo App Started!");

  int err = bsdlib_init();
  if (err) {
    printk("Failed to initialize bsdlib!");
    return;
  }

  err = at_cmd_init();
  if (err) {
    printk("Failed to initialize AT commands, err %d\n", err);
    return;
  }

  err = at_notif_init();
  if (err) {
    printk("Failed to initialize AT notifications, err %d\n", err);
    return;
  }

  // requires AT modem interface to be up
  prv_init_device_info();

  // Note: We must provision certs prior to connecting to the LTE network
  // in order to use them
  err = memfault_nrfconnect_port_install_root_certs();
  if (err) {
    return;
  }

  printk("Waiting for network...\n");
  err = lte_lc_init_and_connect();
  if (err) {
    printk("Failed to connect to the LTE network, err %d\n", err);
    return;
  }
  printk("OK\n");
}
