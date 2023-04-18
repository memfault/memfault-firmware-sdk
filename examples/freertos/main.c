//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "console.h"
#include "memfault/components.h"
#include "task.h"

#define EXAMPLE_TASK_STACKS 300

// The FreeRTOS heap
uint8_t ucHeap[configTOTAL_HEAP_SIZE];

static StackType_t heap_task_stack[EXAMPLE_TASK_STACKS];
static StaticTask_t heap_task_tcb;
static StackType_t metrics_task_stack[EXAMPLE_TASK_STACKS];
static StaticTask_t metrics_task_tcb;

// Next two functions from:
// https://github.com/FreeRTOS/FreeRTOS/blob/6f7f9fd9ed56fbb34d5314af21f17cfa571133a7/FreeRTOS/Demo/CORTEX_M3_MPS2_QEMU_GCC/main.c#L156-L204
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize) {
  /* If the buffers to be provided to the Idle task are declared inside this
   * function then they must be declared static - otherwise they will be allocated on
   * the stack and so not exists after this function exits. */
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
   * state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
   * Note that, as the array is necessarily of type StackType_t,
   * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize) {
  /* If the buffers to be provided to the Timer task are declared inside this
   * function then they must be declared static - otherwise they will be allocated on
   * the stack and so not exists after this function exits. */
  static StaticTask_t xTimerTaskTCB;

  /* Pass out a pointer to the StaticTask_t structure in which the Timer
   * task's state will be stored. */
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

  /* Pass out the array that will be used as the Timer task's stack. */
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;

  /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
   * Note that, as the array is necessarily of type StackType_t,
   * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

static void prv_run_heap_task(MEMFAULT_UNUSED void* pvParameters) {
  void* heap_pointers[16] = {0};

  while (true) {
    for (uint8_t i = 0; i < MEMFAULT_ARRAY_SIZE(heap_pointers); i++) {
      heap_pointers[i] = pvPortMalloc((i + 1) * 64);
      vTaskDelay((1000 * 3) / portTICK_PERIOD_MS);
      MEMFAULT_LOG_DEBUG("Allocated %d @ %p\n", (i + 1) * 64, heap_pointers[i]);
    }

    for (uint8_t i = 0; i < MEMFAULT_ARRAY_SIZE(heap_pointers); i++) {
      MEMFAULT_LOG_DEBUG("Deallocating %p\n", heap_pointers[i]);
      vPortFree(heap_pointers[i]);
      vTaskDelay((1000 * 3) / portTICK_PERIOD_MS);
    }
  }
}

static void prv_heap_task(void) {
  xTaskCreateStatic(
    prv_run_heap_task,                    /* Function that implements the task. */
    "Heap Task",                          /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(heap_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                 /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                     /* Priority at which the task is created. */
    heap_task_stack,                      /* Array to use as the task's stack. */
    &heap_task_tcb);
}

static void prv_run_metrics_task(MEMFAULT_UNUSED void* pvParameters) {
  while (true) {
    HeapStats_t stats = {0};

    vPortGetHeapStats(&stats);
    memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(Example_HeapFreeBytes),
                                            stats.xAvailableHeapSpaceInBytes);
    memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(Example_HeapMinFreeBytes),
                                            stats.xMinimumEverFreeBytesRemaining);
    vTaskDelay((1000 * 10) / portTICK_PERIOD_MS);
  }
}

static void prv_metrics_task(void) {
  xTaskCreateStatic(
    prv_run_metrics_task,                    /* Function that implements the task. */
    "Metrics Task",                          /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(metrics_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                    /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                        /* Priority at which the task is created. */
    metrics_task_stack,                      /* Array to use as the task's stack. */
    &metrics_task_tcb);
}

int main(void) {
  memfault_platform_boot();

  prv_heap_task();
  prv_metrics_task();
  console_init();
  // Initialize the FreeRTOS kernel
  vTaskStartScheduler();

  // Should never reach here
  while (1) {
  };
  return 0;
}
