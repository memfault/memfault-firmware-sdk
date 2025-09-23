//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/thread_metrics.h"

#include <string.h>

#include "memfault/ports/zephyr/include_compatibility.h"
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/metrics/metrics.h"
// clang-format on

MEMFAULT_WEAK MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
#if defined(CONFIG_MEMFAULT_METRICS_THREADS_DEFAULTS)
  #if defined(CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE)
  {
    .thread_name = "mflt_http",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_mflt_http_pct_max),
  },
  #endif
  {
    .thread_name = "idle",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
  },
  {
    .thread_name = "sysworkq",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_sysworkq_pct_max),
  }
#endif
);

static uint32_t prv_get_stack_usage_pct(const struct k_thread *thread) {
  size_t free_stack_size;
  k_thread_stack_space_get(thread, &free_stack_size);

  // The stack size is the total stack size, so we need to subtract the free
  // space to get the used space.
  const size_t stack_usage = thread->stack_info.size - free_stack_size;
  return (uint32_t)(stack_usage * 100 * CONFIG_MEMFAULT_METRICS_THREADS_MEMORY_SCALE_FACTOR) /
         thread->stack_info.size;
}

static void prv_record_thread_metrics(const struct k_thread *thread, void *user_data) {
  ARG_UNUSED(user_data);

  const char *name = k_thread_name_get((k_tid_t)thread);
  if (!name || name[0] == '\0') {
    MEMFAULT_LOG_ERROR("No thread name registered for %p", thread);
  }

  // Iterate over the thread list. A blank thread name indicates the end of the list.
  const sMfltZephyrThreadMetricsIndex *thread_metrics = g_memfault_thread_metrics_index;
  while (thread_metrics->thread_name) {
    if (strcmp(thread_metrics->thread_name, thread->name) == 0) {
      uint32_t stack_usage_pct = prv_get_stack_usage_pct(thread);

      memfault_metrics_heartbeat_set_unsigned(thread_metrics->stack_usage_metric_key,
                                              stack_usage_pct);
      break;
    }
    thread_metrics++;
  }
}

void memfault_zephyr_thread_metrics_record(void) {
  k_thread_foreach_unlocked(prv_record_thread_metrics, NULL);
}
