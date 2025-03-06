#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/metrics/metrics.h"

//! Perform tallying of thread metrics for registered threads. Called
//! automatically by the Memfault SDK.
void memfault_zephyr_thread_metrics_record(void);

typedef struct MfltZephyrThreadMetricsIndex {
  // The name of the thread to monitor. Must _exactly_ match the target thread.
  const char *thread_name;

  // The Memfault Heartbeat Metric key for stack usage. Should be defined as
  // follows:
  //  MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(memory_<thread>_pct_max,
  //    kMemfaultMetricType_Unsigned,
  //    CONFIG_MEMFAULT_METRICS_THREADS_MEMORY_SCALE_FACTOR);
  MemfaultMetricId stack_usage_metric_key;
} sMfltZephyrThreadMetricsIndex;

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
//!   },
//! );
extern const sMfltZephyrThreadMetricsIndex g_memfault_thread_metrics_index[];
#define MEMFAULT_METRICS_DEFINE_THREAD_METRICS(...) \
  const sMfltZephyrThreadMetricsIndex g_memfault_thread_metrics_index[] = { __VA_ARGS__, { 0 } }

#ifdef __cplusplus
}
#endif
