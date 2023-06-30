//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdio.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "memfault/components.h"
#include "task.h"

#define CONSOLE_INPUT_STACK_SIZE 300

/* Structure that will hold the TCB of the task being created. */
static StaticTask_t console_input_task;

/* Buffer that the task being created will use as its stack.  Note this is
an array of StackType_t variables.  The size of StackType_t is dependent on
the RTOS port. */
static StackType_t console_input_task_stack[CONSOLE_INPUT_STACK_SIZE];

static int prv_send_char(char c) {
  int sent = putchar(c);
  fflush(stdout);
  return sent;
}

/* Function that implements the task being created. */
static void prv_console_input_task(MEMFAULT_UNUSED void *pvParameters) {
  while (true) {
    char c;
    if (read(0, &c, sizeof(c))) {
      memfault_demo_shell_receive_char((char)c);
    } else {
      // sleep a bit if we didn't just receive a character, to prevent 100% CPU
      // utilization in this thread.
      vTaskDelay((10) / portTICK_PERIOD_MS);
    }
  }
}

void console_init(void) {
  xTaskCreateStatic(
    prv_console_input_task,                        /* Function that implements the task. */
    "Console Input",                               /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(console_input_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                          /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                              /* Priority at which the task is created. */
    console_input_task_stack,                      /* Array to use as the task's stack. */
    &console_input_task);

  //! We will use the Memfault CLI shell as a very basic debug interface
  const sMemfaultShellImpl impl = {
    .send_char = prv_send_char,
  };
  memfault_demo_shell_boot(&impl);
}
