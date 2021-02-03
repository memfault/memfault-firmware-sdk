//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/ports/zephyr/http.h"

#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"

#include <init.h>
#include <kernel.h>
#include <random/rand32.h>
#include <zephyr.h>

static void prv_metrics_work_handler(struct k_work *work) {
  if (!memfault_packetizer_data_available()) {
    return;
  }

  MEMFAULT_LOG_DEBUG("POSTing Memfault Data");
  memfault_zephyr_port_post_data();
}

K_WORK_DEFINE(s_upload_timer_work, prv_metrics_work_handler);

static void prv_timer_expiry_handler(struct k_timer *dummy) {
  k_work_submit(&s_upload_timer_work);
}

K_TIMER_DEFINE(s_upload_timer, prv_timer_expiry_handler, NULL);

static int prv_background_upload_init() {
  const uint32_t interval_secs = CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS;

  // randomize the first post to spread out the reporting of
  // information from a fleet of devices that all reboot at once
  const uint32_t duration_secs = 60 + (sys_rand32_get() % interval_secs);

  k_timer_start(&s_upload_timer, K_SECONDS(duration_secs), K_SECONDS(interval_secs));
  return 0;
}

SYS_INIT(prv_background_upload_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
