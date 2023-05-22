/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
  #define WITH_TASKS_INFO 1
#endif

static void register_free();
static void register_heap_dump();
static void register_restart();
#if WITH_TASKS_INFO
static void register_tasks();
#endif

void register_system() {
  register_free();
  register_heap_dump();
  register_restart();
#if WITH_TASKS_INFO
  register_tasks();
#endif
}

/** 'restart' command restarts the program */

static int restart(int argc, char** argv) {
  ESP_LOGI(__func__, "Restarting");
  esp_restart();
}

static void register_restart() {
  const esp_console_cmd_t cmd = {
    .command = "restart",
    .help = "Restart the program",
    .hint = NULL,
    .func = &restart,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'free' command prints available heap memory */

static int free_mem(int argc, char** argv) {
  printf("%" PRIu32 "\n", esp_get_free_heap_size());
  return 0;
}

static int dump_heap(int argc, char** argv) {
  heap_caps_print_heap_info(0);
  return 0;
}

static void register_free() {
  const esp_console_cmd_t cmd = {
    .command = "free",
    .help = "Get the total size of heap memory available",
    .hint = NULL,
    .func = &free_mem,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void register_heap_dump() {
  const esp_console_cmd_t cmd = {
    .command = "heap_dump",
    .help = "Dump the current heap stats",
    .hint = NULL,
    .func = &dump_heap,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'tasks' command prints the list of tasks and related information */
#if WITH_TASKS_INFO

static int tasks_info(int argc, char** argv) {
  const size_t bytes_per_task = 40; /* see vTaskList description */
  char* task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
  if (task_list_buffer == NULL) {
    ESP_LOGE(__func__, "failed to allocate buffer for vTaskList output");
    return 1;
  }
  fputs("Task Name\tStatus\tPrio\tHWM\tTask Number\n", stdout);
  vTaskList(task_list_buffer);
  fputs(task_list_buffer, stdout);
  free(task_list_buffer);
  return 0;
}

static void register_tasks() {
  const esp_console_cmd_t cmd = {
    .command = "tasks",
    .help = "Get information about running tasks",
    .hint = NULL,
    .func = &tasks_info,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

#endif  // WITH_TASKS_INFO
