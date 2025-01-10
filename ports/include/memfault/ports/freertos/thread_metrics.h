#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief

#include "memfault/metrics/metrics.h"
#include "memfault/ports/freertos/metrics.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct MfltFreeRTOSTaskMetricsIndex {
  // The name of the task to monitor. Must _exactly_ match the target task.
  // FreeRTOS task names are limited to configMAX_TASK_NAME_LEN bytes in length,
  // be sure to test the string matches exactly for each registered task.
  const char *thread_name;

  // The Memfault Heartbeat Metric key for stack usage. Should be defined as
  // follows:
  //  MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(memory_<task>_pct_max,
  //    kMemfaultMetricType_Unsigned,
  //    MEMFAULT_METRICS_THREADS_MEMORY_SCALE_FACTOR);
  MemfaultMetricId stack_usage_metric_key;
} sMfltFreeRTOSTaskMetricsIndex;

//! This data structure can be initialized by the user to specify which threads
//! should be recorded in the thread metrics. A weak definition is provided that
//! records the idle + sysworkq threads only.
//!
//! The array must contain a blank entry at the end to terminate the list.
//! An example initialization looks like this:
//!
//! MEMFAULT_METRICS_DEFINE_THREAD_METRICS (
//!   {
//!     .thread_name = "idle",
//!     .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
//!   },
//!   {
//!     .thread_name = "sysworkq",
//!     .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_sysworkq_pct_max),
//!   }
//! );
extern const sMfltFreeRTOSTaskMetricsIndex g_memfault_thread_metrics_index[];
#define MEMFAULT_METRICS_DEFINE_THREAD_METRICS(...) \
  const sMfltFreeRTOSTaskMetricsIndex g_memfault_thread_metrics_index[] = { __VA_ARGS__, { 0 } };

//! Perform tallying of thread metrics for registered threads. Called internally
//! by memfault_freertos_port_task_runtime_metrics().
void memfault_freertos_port_thread_metrics(void);
#ifdef __cplusplus
}
#endif  // __cplusplus
