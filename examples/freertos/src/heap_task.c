//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief Example heap task

#include "heap_task.h"

#include <stdint.h>

#include "FreeRTOS.h"
#include "memfault/components.h"

#define EXAMPLE_TASK_STACKS 300

// The FreeRTOS heap
uint8_t ucHeap[configTOTAL_HEAP_SIZE];

static StackType_t heap_task_stack[EXAMPLE_TASK_STACKS];
static StaticTask_t heap_task_tcb;

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

void heap_task_init(void) {
  xTaskCreateStatic(
    prv_run_heap_task,                    /* Function that implements the task. */
    "Heap Task",                          /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(heap_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                 /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                     /* Priority at which the task is created. */
    heap_task_stack,                      /* Array to use as the task's stack. */
    &heap_task_tcb);
}
