//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#include "memfault/ports/zephyr/version.h"

// for zephyr 3.5.0+, use random.h instead of rand32.h
#if MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 4)
#include MEMFAULT_ZEPHYR_INCLUDE(random/random.h)
#else
#include MEMFAULT_ZEPHYR_INCLUDE(random/rand32.h)
#endif

#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/ports/zephyr/http.h"
// clang-format on

static bool s_mflt_upload_enabled = true;

#if CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE
static K_THREAD_STACK_DEFINE(memfault_http_stack_area,
                             CONFIG_MEMFAULT_HTTP_DEDICATED_WORKQUEUE_STACK_SIZE);
static struct k_work_q memfault_http_work_q;
#endif

#if !defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
// Runtime configurable
static bool s_mflt_upload_logs;
#endif

#if !defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
void memfault_zephyr_port_http_periodic_upload_logs(bool enable) {
  s_mflt_upload_logs = enable;
}
#endif

// API to enable/disable all periodic uploads at runtime
void memfault_zephyr_port_http_periodic_upload_enable(bool enable) {
  s_mflt_upload_enabled = enable;
}

static void prv_periodic_upload_work_handler(struct k_work *work) {
  if (!s_mflt_upload_enabled) {
    return;
  }

#if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_LOGS)
  MEMFAULT_LOG_DEBUG("Triggering log collection");
  memfault_log_trigger_collection();
#else
  if (s_mflt_upload_logs) {
    MEMFAULT_LOG_DEBUG("Triggering log collection");
    memfault_log_trigger_collection();
  }
#endif

  if (!memfault_packetizer_data_available()) {
    MEMFAULT_LOG_DEBUG("No Memfault data available");
    return;
  }

  MEMFAULT_LOG_DEBUG("POSTing Memfault Data");
  ssize_t rv = memfault_zephyr_port_post_data_return_size();
  if (rv > 0) {
    MEMFAULT_LOG_DEBUG("Uploaded %zd bytes of Memfault data", rv);
  }
}

K_WORK_DEFINE(s_upload_timer_work, prv_periodic_upload_work_handler);

static void prv_timer_expiry_handler(struct k_timer *dummy) {
#if CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE
  k_work_submit_to_queue(&memfault_http_work_q, &s_upload_timer_work);
#else
  k_work_submit(&s_upload_timer_work);
#endif
}

K_TIMER_DEFINE(s_upload_timer, prv_timer_expiry_handler, NULL);

static int prv_background_upload_init() {
  const uint32_t interval_secs = CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS;

  // Randomize the first post to spread out the reporting of
  // information from a fleet of devices that all reboot at once.
  // For very low values of CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS
  // cut minimum delay of first post for better testing/demoing experience.
  const uint32_t duration_secs_minimum = interval_secs >= 60 ? 60 : 5;
  const uint32_t duration_secs = duration_secs_minimum + (sys_rand32_get() % interval_secs);

  k_timer_start(&s_upload_timer, K_SECONDS(duration_secs), K_SECONDS(interval_secs));
  MEMFAULT_LOG_INFO("Periodic background upload scheduled - initial delay=%ds period=%ds",
                    (int)duration_secs, (int)interval_secs);

#if CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE
  struct k_work_queue_config config = {
    .name = "mflt_http",
    .no_yield = false,
  };

  k_work_queue_start(&memfault_http_work_q, memfault_http_stack_area,
                     K_THREAD_STACK_SIZEOF(memfault_http_stack_area),
                     K_HIGHEST_APPLICATION_THREAD_PRIO, &config);
#endif
  return 0;
}

SYS_INIT(prv_background_upload_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
