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
}
