//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "bsp.h"

#include "qf_port.h"
#include "qf.h"

// Product-specific Memfault dependencies
#include "memfault/core/debug_log.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/demo/shell.h"
#include "memfault/http/http_client.h"

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "qp-main",
    .software_version = "1.0.0",
    .hardware_version = "stm32f407g-disc1",
  };
}

sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "<YOUR PROJECT KEY HERE>",
};

void main(void) {
  bsp_init();

  MEMFAULT_LOG_INFO("Memfault QP demo app started...");
  memfault_demo_shell_boot(&(sMemfaultShellImpl){
      .send_char = bsp_send_char_over_uart,
  });

  QF_init();
  QF_run();
}

void QF_onStartup(void) {
}

void QF_onCleanup(void) {
}

void QV_onIdle(void) {
  char c;
  if (bsp_read_char_from_uart(&c)) {
    memfault_demo_shell_receive_char(c);
  }
}

// void Q_onAssert(char const *module, int loc) {
//   NOTE: After applying the ports/qp/qassert.h.patch and ports/qp/qf_pkg.h.patch files from the Memfault Firmware SDK,
//   the Q_onAssert function will no longer be necessary. Instead, Memfault's assertion handling will be used.
//   To ensure that no code relies on Q_onAssert, we recommend removing the Q_onAssert function and replace any usage
//   of it with the MEMFAULT_ASSERT macro from memfault/panics/assert.h.
// }
