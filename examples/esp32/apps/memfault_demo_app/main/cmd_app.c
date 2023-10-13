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
#include "memfault/components.h"
#include "sdkconfig.h"

//! cause the task watchdog to fire by deadlocking the example task
static int test_task_watchdog(int argc, char** argv) {
  (void)argc, (void)argv;
  ESP_LOGI(__func__, "Setting the example_task to stuck, will cause the task watchdog to fire");

  xSemaphoreTakeRecursive(g_example_task_lock, portMAX_DELAY);

  return 0;
}

static int prv_coredump_size(int argc, char** argv) {
  (void)argc, (void)argv;

  sMfltCoredumpStorageInfo storage_info = {0};
  memfault_platform_coredump_storage_get_info(&storage_info);
  const size_t size_needed = memfault_coredump_storage_compute_size_required();
  printf("coredump storage capacity: %zuB\n", storage_info.size);
  printf("coredump size required: %zuB\n", size_needed);
  return 0;
}

void register_app(void) {
  const esp_console_cmd_t test_watchdog_cmd = {
    .command = "test_task_watchdog",
    .help = "Demonstrate the task watchdog tripping on a deadlock",
    .hint = NULL,
    .func = &test_task_watchdog,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&test_watchdog_cmd));

  const esp_console_cmd_t coredump_size_cmd = {
    .command = "coredump_size",
    .help = "Print the coredump storage capacity and the capacity required",
    .hint = NULL,
    .func = &prv_coredump_size,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&coredump_size_cmd));
}
