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

//! cause the task watchdog to fire by deadlocking the example task
static int test_task_watchdog(int argc, char** argv) {
  (void)argc, (void)argv;
  ESP_LOGI(__func__, "Setting the example_task to stuck, will cause the task watchdog to fire");

  xSemaphoreTakeRecursive(g_example_task_lock, portMAX_DELAY);

  return 0;
}

static void register_test_task_watchdog(void) {
  const esp_console_cmd_t cmd = {
    .command = "test_task_watchdog",
    .help = "Demonstrate the task watchdog tripping on a deadlock",
    .hint = NULL,
    .func = &test_task_watchdog,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void register_app(void) {
  register_test_task_watchdog();
}
