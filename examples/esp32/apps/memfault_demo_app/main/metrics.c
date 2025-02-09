//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Example application metrics.

#include "esp_idf_version.h"
#include "memfault/ports/freertos/thread_metrics.h"

// We test this application in CI in a "zero-config" form, by removing
// memfault_platform_config.h. This enables the default set of thread metrics,
// so to avoid a name conflict, don't enable them here.
#if !MEMFAULT_METRICS_THREADS_DEFAULTS

  #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
// Provide callbacks to get the IDLE task handle for each core
static TaskHandle_t prv_get_idle0_task_handle_for_core(void) {
  return xTaskGetIdleTaskHandleForCPU(0);
}
    #if !defined(CONFIG_FREERTOS_UNICORE)
static TaskHandle_t prv_get_idle1_task_handle_for_core(void) {
  return xTaskGetIdleTaskHandleForCPU(1);
}
    #endif
  #endif

MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
  {
    .thread_name = "main",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_main_pct_max),
  },
  #if defined(CONFIG_FREERTOS_UNICORE)
  {
    .thread_name = "IDLE",
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
    .get_task_handle = prv_get_idle0_task_handle_for_core,
    #endif
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle0_pct_max),
  },
  #else
  {
    .thread_name = "IDLE0",
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
    .get_task_handle = prv_get_idle0_task_handle_for_core,
    #endif
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle0_pct_max),
  },
  {
    .thread_name = "IDLE1",
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
    .get_task_handle = prv_get_idle1_task_handle_for_core,
    #endif
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle1_pct_max),
  },
  #endif
  {
    .thread_name = "example_task",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_example_task_pct_max),
  },
  {
    .thread_name = "tiT",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_tit_pct_max),
  },
  {
    .thread_name = "mflt_periodic_u",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_mflt_periodic_u_pct_max),
  },
  {
    .thread_name = "Tmr Svc",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_tmr_svc_pct_max),
  },
  #if !defined(CONFIG_FREERTOS_UNICORE)
  {
    .thread_name = "ipc0",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_ipc0_pct_max),
  },
  {
    .thread_name = "ipc1",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_ipc1_pct_max),
  },
  #endif
  {
    .thread_name = "ota",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_ota_pct_max),
  },
  {
    .thread_name = "sys_evt",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_sys_evt_pct_max),
  },
  {
    .thread_name = "wifi",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_wifi_pct_max),
  },
  {
    .thread_name = "esp_timer",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_esp_timer_pct_max),
  },
  {
    .thread_name = "overflow_task",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_overflow_task_pct_max),
  });
#endif  // !MEMFAULT_METRICS_THREADS_DEFAULTS
