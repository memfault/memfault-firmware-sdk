//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Metrics for QEMU sample app.

#include "memfault/ports/zephyr/thread_metrics.h"

//! Set the list of threads to monitor for stack usage
MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
#if defined(CONFIG_MEMFAULT_METRICS_THREADS_DEFAULTS)
  {
    .thread_name = "idle",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
  },
  {
    .thread_name = "sysworkq",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_sysworkq_pct_max),
  },
#endif
  {
    .thread_name = "main",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_main_pct_max),
  },
  {
    .thread_name = "shell_uart",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_shell_uart_pct_max),
  });
