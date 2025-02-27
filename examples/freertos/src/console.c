//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "memfault/components.h"
#include "task.h"

#define CONSOLE_INPUT_STACK_SIZE 500

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

// shell extensions
#if defined(MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS)
static int prv_freertos_vassert_cmd(int argc, char *argv[]) {
  (void)argc, (void)argv;
  printf("Triggering vAssertCalled!\n");
  configASSERT(0);
  return 0;
}

// print all task information
static int prv_freertos_tasks_cmd(int argc, char *argv[]) {
  (void)argc, (void)argv;
  size_t task_count = uxTaskGetNumberOfTasks();
  // 100 bytes per task should be enough™️
  char *write_buf = pvPortMalloc(task_count * 100);

  vTaskList(write_buf);

  puts(write_buf);

  vPortFree(write_buf);

  return 0;
}

static int prv_fake_fw_update_error_assert_cmd(int argc, char *argv[]) {
  (void)argc, (void)argv;
  printf("Triggering fake firmware update error assert!\n");
  MEMFAULT_ASSERT_WITH_REASON(0, kMfltRebootReason_FirmwareUpdateError);
  return 0;
}

MEMFAULT_WEAK int memfault_metrics_session_start(eMfltMetricsSessionIndex session_key) {
  (void)session_key;
  MEMFAULT_LOG_RAW("Disabled. metrics component integration required");
  return 0;
}

MEMFAULT_WEAK int memfault_metrics_session_end(eMfltMetricsSessionIndex session_key) {
  (void)session_key;
  MEMFAULT_LOG_RAW("Disabled. metrics component integration required");
  return 0;
}

MEMFAULT_WEAK int memfault_metrics_heartbeat_set_string(MemfaultMetricId key, const char *value) {
  (void)key;
  (void)value;
  MEMFAULT_LOG_RAW("Disabled. metrics component integration required");
  return 0;
}

static int prv_session(int argc, char *argv[]) {
  const char *cmd_name_string = "UNKNOWN";
  if (argc > 1) {
    cmd_name_string = argv[1];
  }

  printf("Executing a test metrics session named 'cli'\n");
  MEMFAULT_METRICS_SESSION_START(cli);
  // API v1
  // MEMFAULT_METRIC_SET_STRING(cli_cmd_name, cmd_name_string);
  // API v2
  MEMFAULT_METRIC_SESSION_SET_STRING(cmd_name, cli, cmd_name_string);
  MEMFAULT_METRICS_SESSION_END(cli);

  return 0;
}

static int prv_spin_cpu(int argc, char *argv[]) {
  uint32_t cycle_count = UINT32_MAX;
  if (argc > 1) {
    char *end = NULL;
    cycle_count = strtoul(argv[1], &end, 10);
    if (end == argv[1]) {
      MEMFAULT_LOG_RAW("Invalid cycle count");
      cycle_count = UINT32_MAX;
    }
  }

  MEMFAULT_LOG_RAW("Spinning CPU for %lu cycles", cycle_count);
  for (uint32_t i = 0; i < cycle_count; i++) { }
  MEMFAULT_LOG_RAW("Spinning completed, check cpu_usage_pct");

  return 0;
}

static int prv_session_crash(int argc, char *argv[]) {
  (void)argc, (void)argv;

  MEMFAULT_METRICS_SESSION_START(cli);
  printf("Triggering session start + crash!\n");

  MEMFAULT_ASSERT(0);

  return 0;
}

static int prv_leak_memory(int argc, char *argv[]) {
  (void)argc, (void)argv;

  // Allocate memory and leak it
  if (argc > 1) {
    unsigned long size = strtoul(argv[1], NULL, 0);
    void *leaked_memory = malloc(size);
    if (leaked_memory == NULL) {
      MEMFAULT_LOG_RAW("Failed to allocate memory");
      return -1;
    }
  }

  return 0;
}

static int prv_wdg(int argc, char *argv[]) {
  (void)argc, (void)argv;

  MEMFAULT_LOG_ERROR("Simulating watchdog crash");

  MEMFAULT_CRASH_WITH_REASON(kMfltRebootReason_TaskWatchdog);

  return 0;
}

static const sMemfaultShellCommand s_freertos_example_shell_extension_list[] = {
  {
    .command = "freertos_vassert",
    .handler = prv_freertos_vassert_cmd,
    .help = "Example command",
  },
  {
    .command = "freertos_tasks",
    .handler = prv_freertos_tasks_cmd,
    .help = "Print all FreeRTOS tasks",
  },
  {
    .command = "fwup_assert",
    .handler = prv_fake_fw_update_error_assert_cmd,
    .help = "Trigger fake firmware update error assert",
  },
  {
    .command = "session",
    .handler = prv_session,
    .help = "Execute a test metrics session named 'cli'",
  },
  {
    .command = "spin_cpu",
    .handler = prv_spin_cpu,
    .help = "Spin CPU to increase cpu_usage_pct metric",
  },
  {
    .command = "session_crash",
    .handler = prv_session_crash,
    .help = "Trigger a crash during a session",
  },
  {
    .command = "leak",
    .handler = prv_leak_memory,
    .help = "Allocate memory and leak it. Usage: leak <num_bytes>",
  },
  {
    .command = "wdg",
    .handler = prv_wdg,
    .help = "Simulated watchdog crash",
  }
};
#endif

void console_task_init(void) {
  xTaskCreateStatic(
    prv_console_input_task,                        /* Function that implements the task. */
    "Console Input",                               /* Text name for the task. */
    MEMFAULT_ARRAY_SIZE(console_input_task_stack), /* Number of indexes in the xStack array. */
    NULL,                                          /* Parameter passed into the task. */
    tskIDLE_PRIORITY,                              /* Priority at which the task is created. */
    console_input_task_stack,                      /* Array to use as the task's stack. */
    &console_input_task);

  //! We will use the Memfault CLI shell as a very basic debug interface
#if defined(MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS)
  memfault_shell_command_set_extensions(
    s_freertos_example_shell_extension_list,
    MEMFAULT_ARRAY_SIZE(s_freertos_example_shell_extension_list));
#endif
  const sMemfaultShellImpl impl = {
    .send_char = prv_send_char,
  };
  memfault_demo_shell_boot(&impl);
}
