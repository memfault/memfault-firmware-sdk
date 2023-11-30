//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief Metrics for the application

#include "metrics.h"

#include "FreeRTOS.h"
#include "memfault/components.h"

#define EXAMPLE_TASK_STACKS 300

static StackType_t metrics_task_stack[EXAMPLE_TASK_STACKS];
static StaticTask_t metrics_task_tcb;

static void prv_run_metrics_task(MEMFAULT_UNUSED void* pvParameters) {
  while (true) {
    HeapStats_t stats = {0};

    vPortGetHeapStats(&stats);
    MEMFAULT_METRIC_SET_UNSIGNED(Example_HeapFreeBytes, stats.xAvailableHeapSpaceInBytes);
    MEMFAULT_METRIC_SET_UNSIGNED(Example_HeapMinFreeBytes, stats.xMinimumEverFreeBytesRemaining);
    vTaskDelay((1000 * 10) / portTICK_PERIOD_MS);
  }
}

void metrics_task_init(void) {
  xTaskCreateStatic(
    prv_run_metrics_task,                    /* Function that implements the task. */
    "Metrics Task",                          /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(metrics_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                    /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                        /* Priority at which the task is created. */
    metrics_task_stack,                      /* Array to use as the task's stack. */
    &metrics_task_tcb);
}

void memfault_metrics_heartbeat_collect_sdk_data(void) {
  MEMFAULT_STATIC_ASSERT(
    MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS <= (60 * 60),
    "Heartbeat must be an hour or less for runtime metrics to mitigate counter overflow");

  memfault_freertos_port_task_runtime_metrics();
}
