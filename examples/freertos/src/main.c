//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#if defined(DEBUG_COMPACT_LOGS)
  #include <stdio.h>
#endif
#include <sys/times.h>

#include "FreeRTOS.h"
#include "compact_log.h"
#include "console.h"
#include "heap_task.h"
#include "memfault/components.h"
#include "metrics.h"
#include "task.h"

// Next two functions from:
// https://github.com/FreeRTOS/FreeRTOS/blob/6f7f9fd9ed56fbb34d5314af21f17cfa571133a7/FreeRTOS/Demo/CORTEX_M3_MPS2_QEMU_GCC/main.c#L156-L204
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
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

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
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

unsigned long ulGetRunTimeCounterValue(void) {
  struct tms xTimes;
  times(&xTimes);

  return (unsigned long)xTimes.tms_utime;
}

void compact_log_c_example(void) {
#if MEMFAULT_COMPACT_LOG_ENABLE
  MEMFAULT_COMPACT_LOG_SAVE(kMemfaultPlatformLogLevel_Info,
                            "This is a compact log example from c "
                            // clang-format off
    "%d"  " %" PRIu64    " %f"  " %f"   " %s"   " %s"             " %s"           " %s"                    " %s",
    1234, (uint64_t)0x7, 1.0f,  2.0,    "1212", (uint8_t*)"1212", (int8_t*)"1212", (const uint8_t*)"1212", (const int8_t*)"1212"
//  ^int  ^uint64_t      ^float ^double ^char*  ^unsigned char*   ^signed char*    ^const unsigned char*   ^const signed char*
                            // clang-format on
  );
#endif  // MEMFAULT_COMPACT_LOG_ENABLE
}

int main(void) {
  memfault_platform_boot();

  heap_task_init();

#if defined(MEMFAULT_COMPONENT_metrics_)
  metrics_task_init();
#endif

  console_task_init();

  compact_log_c_example();
  compact_log_cpp_example();

  // Enable this to export compact log data to stdio. This is intended only for
  // testing deserialization of the compact log example above and is not
  // typically useful.
#if defined(DEBUG_COMPACT_LOGS)
  printf(">> exporting logs...\n");
  memfault_log_export_logs();
#endif

  // Initialize the FreeRTOS kernel
  vTaskStartScheduler();

  // Should never reach here
  while (1) { };
  return 0;
}
