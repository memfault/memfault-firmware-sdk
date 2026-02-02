//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief

#include "memfault/ports/freertos/metrics.h"

#if MEMFAULT_FREERTOS_COLLECT_THREAD_METRICS

  #include <stdint.h>
  #include <string.h>

  #include "memfault/metrics/metrics.h"
  #include "memfault/ports/freertos/thread_metrics.h"

  #if !defined(MEMFAULT_METRICS_HEARTBEAT_FREERTOS_CONFIG_DEF)
    #error \
      "Please include memfault_metrics_heartbeat_freertos_config.def in your memfault_metrics_heartbeat_config.def file"
  #endif

  #ifdef MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
  #else
    #include "FreeRTOS.h"
    #include "task.h"
  #endif

  #if !configRECORD_STACK_HIGH_ADDRESS && !(configUSE_TRACE_FACILITY && INCLUDE_uxTaskGetStackHighWaterMark)
    #error "configRECORD_STACK_HIGH_ADDRESS must be set to 1 in FreeRTOSConfig.h"
  #endif

  #define DEBUG_ 0
  #if DEBUG_
    #include <stdio.h>
    #define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
  #else
    #define DEBUG_PRINTF(fmt, ...)
  #endif

  #if MEMFAULT_METRICS_THREADS_DEFAULTS_INDEX
    // There is a bug in older versions of GCC when using -std=gnu11 that prevents
    // the use of compound designated initializers.
    #if (__STDC_VERSION__ < 201100L) || (__GNUC__ >= 5)
MEMFAULT_WEAK MEMFAULT_METRICS_DEFINE_THREAD_METRICS(
      #if MEMFAULT_METRICS_THREADS_DEFAULTS
  {
    .thread_name = "IDLE",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_idle_pct_max),
  },
  {
    .thread_name = "Tmr Svc",
    .stack_usage_metric_key = MEMFAULT_METRICS_KEY(memory_tmr_svc_pct_max),
  }
      #endif  // MEMFAULT_METRICS_THREADS_DEFAULTS
);
    #else   // (__STDC_VERSION__ < 201100L) || (__GNUC__ >= 5)
MEMFAULT_WEAK const sMfltFreeRTOSTaskMetricsIndex g_memfault_thread_metrics_index[] = {
  { 0 },
};
    #endif  // (__STDC_VERSION__ < 201100L) || (__GNUC__ >= 5)
  #endif    // MEMFAULT_METRICS_THREADS_DEFAULTS_INDEX

  #if MEMFAULT_FREERTOS_VERSION_GTE(10, 0, 0) || defined(MEMFAULT_FREERTOS_PXENDOFSTACK_AVAILABLE)
// Unfortunately we must break the opaque pointer to the TCB and access the
// pxEndOfStack field directly. The simplest approach is to access the task
// name address in the TCB, then offset to the location of the pxEndOfStack
// field, which has a reliable location in the versions of FreeRTOS we
// support.
// https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/V10.0.0/FreeRTOS/Source/tasks.c#L281-L285
// https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/V10.4.6/tasks.c#L267-L271
// https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/6f63da20b3c9e1408e7c8007d3427b75878cbd64/tasks.c#L267-L271
// https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/V10.6.2/tasks.c#L271-L275
struct MfltFreeRTOSTCB {
  char pcTaskName[configMAX_TASK_NAME_LEN];
    // Two cases require additional adjustment:
    // 1. vanilla FreeRTOS SMP versions (i.e V11.0.0+) that enable
    //    configUSE_TASK_PREEMPTION_DISABLE:
    //    https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/V11.0.0/tasks.c#L373
    // 2. ESP-IDF fork, with configNUMBER_OF_CORES > 1:
    //    https://github.com/espressif/esp-idf/blob/v5.3/components/freertos/FreeRTOS-Kernel/tasks.c#L408
    #if MEMFAULT_FREERTOS_VERSION_GTE(11, 0, 0) || defined(ESP_PLATFORM)
      #if (configUSE_TASK_PREEMPTION_DISABLE == 1) || (configNUMBER_OF_CORES > 1)
  BaseType_t xPreemptionDisable_or_xCoreID;
      #elif defined(MEMFAULT_USE_ESP32_FREERTOS_INCLUDE)
        // ESP-IDF TCB_t unconditionally included the xCoreID until v5.3.0:
        // https://github.com/espressif/esp-idf/commit/439c7c4261837b4563f278b095c77accb266a8c1
        #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
  BaseType_t xCoreID;
        #endif
      #endif
    #endif  // MEMFAULT_FREERTOS_VERSION_GTE(11, 0, 0) || defined(ESP_PLATFORM)
  uint32_t pxEndOfStack;
};
  #else
    #error \
      "Unsupported FreeRTOS version, please contact https://mflt.io/contact-support for assistance"
  #endif

static uint32_t prv_get_stack_usage_pct_for_thread(const TaskHandle_t task_handle) {
  // Read task info to get the stack unused bytes and base address
  TaskStatus_t task_status;
  vTaskGetInfo(task_handle, &task_status, pdTRUE, eRunning);
  const uint32_t stack_unused_bytes = task_status.usStackHighWaterMark * sizeof(StackType_t);
  const uint32_t stack_base = (uint32_t)task_status.pxStackBase;

  // Access the TCB structure from the task name address to get the stack_end
  char *task_name = (char *)task_status.pcTaskName;
  struct MfltFreeRTOSTCB *tcb = (struct MfltFreeRTOSTCB *)(task_name);
  const uint32_t stack_end = tcb->pxEndOfStack;

  // Calculate stack size and usage percentage
  const uint32_t stack_size = stack_end - stack_base + sizeof(StackType_t);
  const uint32_t stack_used_bytes = stack_size - stack_unused_bytes;
  const uint32_t stack_pct =
    (100 * MEMFAULT_METRICS_THREADS_MEMORY_SCALE_FACTOR * stack_used_bytes) / stack_size;

  DEBUG_PRINTF("Task: %s\n", task_status.pcTaskName);
  DEBUG_PRINTF("  Stack End: 0x%08lx\n", stack_end);
  DEBUG_PRINTF("  Stack Base: 0x%08lx\n", stack_base);
  DEBUG_PRINTF("  Stack Size: %lu\n", stack_size);
  DEBUG_PRINTF("  Stack Used Bytes: %lu\n", stack_used_bytes);
  DEBUG_PRINTF("  Stack Unused Bytes: %lu\n", stack_unused_bytes);
  DEBUG_PRINTF("  Stack Usage: %lu.%02lu%%\n", stack_pct / 100, stack_pct % 100);

  return stack_pct;
}

void memfault_freertos_port_thread_metrics(void) {
  const sMfltFreeRTOSTaskMetricsIndex *thread_metrics = g_memfault_thread_metrics_index;
  while (thread_metrics->thread_name) {
    DEBUG_PRINTF("Thread: %s\n", thread_metrics->thread_name);
    TaskHandle_t task_handle = NULL;
    if (thread_metrics->get_task_handle) {
      task_handle = thread_metrics->get_task_handle();
    }
    if (task_handle == NULL) {
      task_handle = xTaskGetHandle(thread_metrics->thread_name);
    }
    if (task_handle != NULL) {
      memfault_metrics_heartbeat_set_unsigned(thread_metrics->stack_usage_metric_key,
                                              prv_get_stack_usage_pct_for_thread(task_handle));
    } else {
      DEBUG_PRINTF("  Thread not found\n");
    }
    thread_metrics++;
  }
}

#endif  // MEMFAULT_FREERTOS_COLLECT_THREAD_METRICS
