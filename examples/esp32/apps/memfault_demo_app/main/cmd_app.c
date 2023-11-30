//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! Miscellaneous commands specific to this example app
//!

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cmd_decl.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "memfault/components.h"
#include "sdkconfig.h"

#if MEMFAULT_TASK_WATCHDOG_ENABLE
//! cause the task watchdog to fire by deadlocking the example task
static int test_task_watchdog(int argc, char** argv) {
  (void)argc, (void)argv;
  ESP_LOGI(__func__, "Setting the example_task to stuck, will cause the task watchdog to fire");

  xSemaphoreTakeRecursive(g_example_task_lock, portMAX_DELAY);

  return 0;
}
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  #if defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK) || \
    !defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE)
    #define OVERFLOW_TASK_STACK_SIZE 4096

static StaticTask_t s_overflow_task_tcb;
static StackType_t s_overflow_task_stack[OVERFLOW_TASK_STACK_SIZE];
static TaskHandle_t s_overflow_task_handle = NULL;

static void prv_trigger_stack_overflow(void) {
    #if defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_PTRVAL)
  // General idea is to allocate a bunch of memory on the stack but not too much
  // to hose FreeRTOS Then yield and the FreeRTOS stack overflow check (on task
  // switch) should kick in due to stack pointer limits

  // Add a new variable to the stack
  uint8_t temp = 0;

  // Calculate bytes remaining between current stack pointer (address of temp)
  // and the lowest address in the stack. Stack is full descending, so subtract
  // 4 bytes to account for currently used location.
  size_t stack_remaining = (uintptr_t)&temp - (uintptr_t)s_overflow_task_stack - 4;
  uint8_t stack_array[stack_remaining];

  // Use the array in some manner to prevent optimization
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(stack_array); i++) {
    stack_array[i] = i;
    taskYIELD();
  }
    #else
  // The canary checks only look at the last bytes (lowest addresses) of the
  // stack Execute a write over the last 32 bytes to trigger the watchpoint
  memset(s_overflow_task_stack, 0, 32);
    #endif  // defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_PTR)
}

/**
 * Function to run stack overflow task, reads the lowest stack address
 * to see if canary values have been clobbered
 */
static void prv_overflow_task(MEMFAULT_UNUSED void *params) {
  while (true) {
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    prv_trigger_stack_overflow();

    // For debugging purposes, print the lowest stack memory to see if canary
    // value is still present
    ESP_LOGD(__func__, "overflow triggered, lowest stack memory contents: %x",
             s_overflow_task_stack[0]);
  }
}

/**
 * Crashes overflow_task by modifying a canary byte in the stack
 */
static int prv_test_stack_overflow(MEMFAULT_UNUSED int argc, MEMFAULT_UNUSED char **argv) {
  ESP_LOGI(__func__, "triggering stack overflow");
  xTaskNotifyGive(s_overflow_task_handle);
  return 0;
}

static void prv_init_stack_overflow_test(void) {
  // Create a task with static memory which we will trigger stack overflow
  // checks on command
  s_overflow_task_handle = xTaskCreateStatic(
    prv_overflow_task, "overflow_task", MEMFAULT_ARRAY_SIZE(s_overflow_task_stack), NULL,
    tskIDLE_PRIORITY, s_overflow_task_stack, &s_overflow_task_tcb);

  const esp_console_cmd_t test_stack_overflow_cmd = {
    .command = "test_stack_overflow",
    .help = "Demonstrate a coredump triggered by stack overflow detection",
    .hint = NULL,
    .func = &prv_test_stack_overflow,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&test_stack_overflow_cmd));
}
  #endif  // defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK) ||
          // !defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE)
#endif    // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)

void register_app(void) {
#if MEMFAULT_TASK_WATCHDOG_ENABLE
  const esp_console_cmd_t test_watchdog_cmd = {
    .command = "test_task_watchdog",
    .help = "Demonstrate the task watchdog tripping on a deadlock",
    .hint = NULL,
    .func = &test_task_watchdog,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&test_watchdog_cmd));
#endif

// Only support the stack overflow test on esp-idf >= 4.3.0
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  #if defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK) || \
    !defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE)
  prv_init_stack_overflow_test();
  #endif  // defined(CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK) ||
          // !defined(CONFIG_FREERTOS_CHECK_STACKOVERFLOW_NONE)
#endif    // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
}
