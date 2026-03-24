//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include "memfault/core/debug_log.h"
#include "memfault/ports/ncs/version.h"
#include "memfault/ports/zephyr/http.h"
#include "modem/nrf_modem_lib.h"

// NRF_MODEM_LIB_ON_INIT() was added in NCS v2.0.0
#if !MEMFAULT_NCS_VERSION_GT(1, 9)
  #error \
    "CONFIG_MEMFAULT_BATTERY_METRICS_BOOT_ON_SYSTEM_INIT requires nRF Connect SDK > 1.9. Please upgrade, or reach out to mflt.io/contact-support for assistance."
#endif

static void prv_install_zephyr_port_root_certs(int ret, MEMFAULT_UNUSED void *ctx) {
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("Modem library init error, ret=%d. Memfault root cert installation skipped.",
                       ret);
    return;
  }

  int err = memfault_zephyr_port_install_root_certs();
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to install root certs, error: %d", err);
    MEMFAULT_LOG_WARN("Certificates can not be provisioned while LTE is active");
  }
}

NRF_MODEM_LIB_ON_INIT(memfault_root_cert_install, prv_install_zephyr_port_root_certs, NULL);
