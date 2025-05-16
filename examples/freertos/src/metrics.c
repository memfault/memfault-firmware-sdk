//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//! @brief Metrics for the application

#include "metrics.h"

#include <malloc.h>

#include "FreeRTOS.h"
#include "memfault/components.h"
#include "memfault/ports/freertos/metrics.h"
#include "memfault/ports/freertos/thread_metrics.h"
#include "task.h"

#define EXAMPLE_TASK_STACKS 300

#define DEBUG_ 0
#if DEBUG_
  #include <stdio.h>
  #define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINTF(fmt, ...)
#endif

#if !defined(MEMFAULT_EXAMPLE_DAILY_HEARTBEAT_ENABLE)
  #define MEMFAULT_EXAMPLE_DAILY_HEARTBEAT_ENABLE 1
#endif

static StackType_t metrics_task_stack[EXAMPLE_TASK_STACKS];
static StaticTask_t metrics_task_tcb;

#if MEMFAULT_EVENT_STORAGE_RESTORE_STATE
// Just a stub, not implemented in this example
bool memfault_event_storage_restore_state(sMfltEventStorageSaveState *state) {
  (void)state;
  return false;
}
#endif

#if MEMFAULT_METRICS_RESTORE_STATE
// Just a stub, not implemented in this example
bool memfault_metrics_restore_state(void *state) {
  (void)state;
  return false;
}
#endif

// This is incompatible with cstd=gnu11 and gcc < 5
#if (__STDC_VERSION__ < 201100L) || (__GNUC__ >= 5)
MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
  {
    .thread_name = "IDLE",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
  },
  {
    .thread_name = "Tmr Svc",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_tmr_svc_pct_max),
  },
  {
    .thread_name = "Console Input",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_console_input_pct_max),
  },
  {
    .thread_name = "📊 Metrics",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_metrics_pct_max),
  },
  {
    .thread_name = "Heap Task",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_heap_task_pct_max),
  });
#endif

static void prv_run_metrics_task(MEMFAULT_UNUSED void *pvParameters) {
  while (true) {
    HeapStats_t stats = { 0 };

    vPortGetHeapStats(&stats);
    MEMFAULT_METRIC_SET_UNSIGNED(FreeRTOS_HeapFreeBytes, stats.xAvailableHeapSpaceInBytes);
    MEMFAULT_METRIC_SET_UNSIGNED(FreeRTOS_HeapMinFreeBytes, stats.xMinimumEverFreeBytesRemaining);

    // per-mille (1/1000) percentage, scaled on ingestion
    const uint32_t heap_pct_max =
      (10000 * (configTOTAL_HEAP_SIZE - stats.xMinimumEverFreeBytesRemaining)) /
      configTOTAL_HEAP_SIZE;
    MEMFAULT_METRIC_SET_UNSIGNED(FreeRTOS_Heap_pct_max, heap_pct_max);
    vTaskDelay((1000 * 10) / portTICK_PERIOD_MS);
  }
}

void metrics_task_init(void) {
  xTaskCreateStatic(
    prv_run_metrics_task,                    /* Function that implements the task. */
    "📊 Metrics",                            /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(metrics_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                    /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                        /* Priority at which the task is created. */
    metrics_task_stack,                      /* Array to use as the task's stack. */
    &metrics_task_tcb);
}

static void prv_collect_libc_heap_usage_metrics(void) {
  // use mallinfo to compute libc heap utilization percentage
  struct mallinfo info = mallinfo();

  extern uint32_t _heap_bottom;
  extern uint32_t _heap_top;

  const uint32_t heap_size = (uint32_t)(&_heap_top - &_heap_bottom);
  const uint32_t heap_used = info.uordblks;
  const uint32_t heap_pct = (10000 * heap_used) / heap_size;

  DEBUG_PRINTF("Heap Usage: %lu/%lu (%lu.%02lu%%)\n", heap_used, heap_size, heap_pct / 100,
               heap_pct % 100);

  MEMFAULT_METRIC_SET_UNSIGNED(memory_pct_max, heap_pct);
}

#if MEMFAULT_EXAMPLE_DAILY_HEARTBEAT_ENABLE
static void prv_daily_heartbeat(void) {
  // Generate a daily_heartbeat session report once a day, based on
  // MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS. This demonstrates a simple way to
  // track metrics over a 24 hour interval.
  MEMFAULT_STATIC_ASSERT((24 * 60 * 60) % MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS == 0,
                         "Heartbeat interval must be an even divisor of a day");
  // For testing, set MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS to a low value (1 second), and
  // uncomment the below line
  // #define DAILY_HEARTBEAT_INTERVAL_COUNT (24)
  #define DAILY_HEARTBEAT_INTERVAL_COUNT ((24 * 60 * 60) / MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS)
  static uint64_t s_last_daily_heartbeat_interval = 0;

  if (++s_last_daily_heartbeat_interval % DAILY_HEARTBEAT_INTERVAL_COUNT == 0) {
    // The first time this is called on boot will be a no-op, since the session
    // has not been started yet
    uint32_t uptime_s = memfault_platform_get_time_since_boot_ms() / 1000;

    if (uptime_s > MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS) {
      MEMFAULT_LOG_INFO("📆 Triggering daily heartbeat");
    }

    // Record a sample metric into the daily session
    MEMFAULT_METRIC_SESSION_SET_UNSIGNED(uptime_s, daily_heartbeat, uptime_s);

    // End the session
    MEMFAULT_METRICS_SESSION_END(daily_heartbeat);

    // Start a new session for the next daily interval
    MEMFAULT_METRICS_SESSION_START(daily_heartbeat);
  }
}
#endif  // MEMFAULT_EXAMPLE_DAILY_HEARTBEAT_ENABLE

void memfault_metrics_heartbeat_collect_data(void) {
  MEMFAULT_LOG_INFO("💓 Heartbeat callback triggered");

  MEMFAULT_STATIC_ASSERT(
    MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS <= (60 * 60),
    "Heartbeat must be an hour or less for runtime metrics to mitigate counter overflow");

#if MEMFAULT_TEST_USE_PORT_TEMPLATE != 1
  memfault_freertos_port_task_runtime_metrics();
#endif
  prv_collect_libc_heap_usage_metrics();

#if MEMFAULT_EXAMPLE_DAILY_HEARTBEAT_ENABLE
  prv_daily_heartbeat();
#endif

  // For demonstration purposes, print the current values. This is not
  // recommended for production.
  memfault_metrics_heartbeat_debug_print();
}
