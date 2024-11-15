//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Example app main

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"
#include <stdlib.h>

#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(devicetree.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/gpio.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

#include "cdr.h"
#include "memfault/components.h"
#include "memfault/ports/zephyr/core.h"
// clang-format on

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_METRICS
  //! Size of memory to allocate on main stack
  //! This requires a larger allocation due to the method used to measure stack usage
  #define STACK_ALLOCATION_SIZE (CONFIG_MAIN_STACK_SIZE >> 2)
  //! Size of memory to allocate on system heap
  #define HEAP_ALLOCATION_SIZE (CONFIG_HEAP_MEM_POOL_SIZE >> 3)
  //! Value to sleep for observing metrics changes
  #define METRICS_OBSERVE_PERIOD MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS

//! Array of heap pointers
static void *heap_ptrs[4] = { NULL };

//! Keep a reference to the main thread for stack info
static struct k_thread *s_main_thread = NULL;
#endif  // CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_METRICS

// Blink code taken from the zephyr/samples/basic/blinky example.
static void blink_forever(void) {
#if DT_NODE_HAS_PROP(DT_ALIAS(led0), gpios)
  /* 1000 msec = 1 sec */
  #define SLEEP_TIME_MS 1000

  /* The devicetree node identifier for the "led0" alias. */
  #define LED0_NODE DT_ALIAS(led0)

  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return;
  }

  while (1) {
    gpio_pin_toggle_dt(&led);
    k_msleep(SLEEP_TIME_MS);
  }
#else
  // no led on this board, just sleep forever
  k_sleep(K_FOREVER);
#endif
}

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-app",
    .software_version =
      CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_SOFTWARE_VERSION "+" ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1,
    .hardware_version = CONFIG_BOARD,
  };
}

#if CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_THREAD_TOGGLE
K_THREAD_STACK_DEFINE(test_thread_stack_area, 1024);
static struct k_thread test_thread;

static void prv_test_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  k_sleep(K_FOREVER);
}

static void prv_test_thread_work_handler(struct k_work *work) {
  ARG_UNUSED(work);

  static bool started = false;
  if (started) {
    LOG_INF("ending test_thread ❌");
    k_thread_abort(&test_thread);
    started = false;
  } else {
    LOG_INF("starting test_thread ✅");
    k_thread_create(&test_thread, test_thread_stack_area,
                    K_THREAD_STACK_SIZEOF(test_thread_stack_area), prv_test_thread_function, NULL,
                    NULL, NULL, 7, 0, K_FOREVER);
    k_thread_name_set(&test_thread, "test_thread");
    k_thread_start(&test_thread);
    started = true;
  }
}
K_WORK_DEFINE(s_test_thread_work, prv_test_thread_work_handler);

//! Timer handlers run from an ISR so we dispatch the heartbeat job to the
//! worker task
static void prv_test_thread_timer_expiry_handler(struct k_timer *dummy) {
  k_work_submit(&s_test_thread_work);
}
K_TIMER_DEFINE(s_test_thread_timer, prv_test_thread_timer_expiry_handler, NULL);

static void prv_init_test_thread_timer(void) {
  k_timer_start(&s_test_thread_timer, K_SECONDS(10), K_SECONDS(10));

  // fire one time right away
  k_work_submit(&s_test_thread_work);
}
#else
static void prv_init_test_thread_timer(void) { }
#endif  // CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_THREAD_TOGGLE

#if CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_METRICS

//! Helper function to collect metric value on main thread stack usage.
static void prv_collect_main_thread_stack_free(void) {
  if (s_main_thread == NULL) {
    return;
  }

  size_t unused = 0;
  int rc = k_thread_stack_space_get(s_main_thread, &unused);
  if (rc == 0) {
    rc = MEMFAULT_METRIC_SET_UNSIGNED(MainStack_MinBytesFree, unused);
    if (rc) {
      LOG_ERR("Error[%d] setting MainStack_MinBytesFree", rc);
    }
  } else {
    LOG_ERR("Error getting thread stack usage[%d]", rc);
  }
}

static void prv_collect_main_thread_run_stats(void) {
  static uint64_t s_prev_main_thread_cycles = 0;
  static uint64_t s_prev_cpu_all_cycles = 0;

  if (s_main_thread == NULL) {
    return;
  }

  k_thread_runtime_stats_t rt_stats_main = { 0 };
  k_thread_runtime_stats_t rt_stats_all = { 0 };

  k_thread_runtime_stats_get(s_main_thread, &rt_stats_main);
  k_thread_runtime_stats_all_get(&rt_stats_all);

  // Calculate difference since last heartbeat
  uint64_t current_main_thread_cycles = rt_stats_main.execution_cycles - s_prev_main_thread_cycles;
  // Note: execution_cycles = idle + non-idle, total_cycles = non-idle
  uint64_t current_cpu_total_cycles = rt_stats_all.execution_cycles - s_prev_cpu_all_cycles;

  // Calculate permille of main thread execution vs total CPU time
  // Multiply permille factor first to avoid truncating lower bits after integer division
  uint32_t main_thread_cpu_time = (current_main_thread_cycles * 1000) / current_cpu_total_cycles;
  MEMFAULT_METRIC_SET_UNSIGNED(main_thread_cpu_time_permille, main_thread_cpu_time);

  // Update previous values
  s_prev_main_thread_cycles = current_main_thread_cycles;
  s_prev_cpu_all_cycles = current_cpu_total_cycles;
}

//! Shell function to exercise example memory metrics
//!
//! This function demonstrates the change in stack and heap memory metrics
//! as memory is allocated and deallocated from these regions.
//!
//! @warning This code uses `memfault_metrics_heartbeat_debug_trigger` which is not intended
//! to be used in production code. This functions use here is solely to demonstrate the metrics
//! values changing. Production applications should rely on the heartbeat timer to trigger
//! collection
static int prv_run_example_memory_metrics(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(shell);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  if (s_main_thread == NULL) {
    return 0;
  }

  // Next two loops demonstrate heap usage metric
  for (size_t i = 0; i < ARRAY_SIZE(heap_ptrs); i++) {
    heap_ptrs[i] = k_malloc(HEAP_ALLOCATION_SIZE);
  }

  // Collect data after allocation
  memfault_metrics_heartbeat_debug_trigger();

  for (size_t i = 0; i < ARRAY_SIZE(heap_ptrs); i++) {
    k_free(heap_ptrs[i]);
    heap_ptrs[i] = NULL;
  }

  // Collect data after deallocation
  memfault_metrics_heartbeat_debug_trigger();
  return 0;
}

SHELL_CMD_REGISTER(memory_metrics, NULL, "Collects runtime memory metrics from application",
                   prv_run_example_memory_metrics);

//! Callback to collect stack usage for specific threads
static void prv_collect_thread_stack_usage_cb(const struct k_thread *thread, void *user_data) {
  // table mapping thread names to metric IDs
  static const struct {
    const char *name;
    MemfaultMetricId id;
  } threads[] = {
    { "shell_uart", MEMFAULT_METRICS_KEY(shell_uart_stack_free_bytes) },
  };

  // scan for a matching thread name
  for (size_t i = 0; i < ARRAY_SIZE(threads); i++) {
    if (strncmp(thread->name, threads[i].name, CONFIG_THREAD_MAX_NAME_LEN) == 0) {
      // get the stack usage and record in the matching metric id
      size_t stack_free_bytes;
      k_thread_stack_space_get(thread, &stack_free_bytes);
      memfault_metrics_heartbeat_set_unsigned(threads[i].id, stack_free_bytes);
    }
  }
}

// Override function to collect the app metric MainStack_MinBytesFree
// and print current metric values
void memfault_metrics_heartbeat_collect_data(void) {
  prv_collect_main_thread_stack_free();
  prv_collect_main_thread_run_stats();

  k_thread_foreach(prv_collect_thread_stack_usage_cb, NULL);

  memfault_metrics_heartbeat_debug_print();
}

//! Helper function to demonstrate changes in stack metrics
static void prv_run_stack_metrics_example(void) {
  volatile uint8_t stack_array[STACK_ALLOCATION_SIZE];
  memset((uint8_t *)stack_array, 0, STACK_ALLOCATION_SIZE);
}
#endif  // CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_METRICS

#if defined(CONFIG_MEMFAULT_FAULT_HANDLER_RETURN)
  #include MEMFAULT_ZEPHYR_INCLUDE(fatal.h)
void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf) {
  printk("User fatal handler invoked with reason: %d. Rebooting!\n", reason);
  memfault_platform_reboot();
}
#endif

static int prv_metrics_session(const struct shell *shell, size_t argc, char **argv) {
  const char *cmd_name_string = "UNKNOWN";
  if (argc > 1) {
    cmd_name_string = argv[1];
  }

  shell_print(shell, "Executing a test metrics session named 'cli'\n");

  MEMFAULT_METRICS_SESSION_START(cli);
  // API v1
  // MEMFAULT_METRIC_SET_STRING(cli_cmd_name, cmd_name_string);
  // API v2
  MEMFAULT_METRIC_SESSION_SET_STRING(cmd_name, cli, cmd_name_string);
  MEMFAULT_METRICS_SESSION_END(cli);

  return 0;
}

SHELL_CMD_REGISTER(metrics_session, NULL, "Trigger a one-time metrics session for testing",
                   prv_metrics_session);

static int prv_session_crash(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(shell);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  LOG_ERR("Triggering crash");

  MEMFAULT_METRICS_SESSION_START(cli);

  MEMFAULT_ASSERT(0);

  return 0;
}

SHELL_CMD_REGISTER(session_crash, NULL, "session crash", prv_session_crash);

#if defined(CONFIG_BBRAM)
  #include <zephyr/drivers/bbram.h>

void memfault_reboot_tracking_load(sMemfaultRebootTrackingStorage *dst) {
  // restore reboot tracking from bbram_read

  // clear the destination buffer- if the bbram restore doesn't work, we won't
  // have reboot tracking data.
  memset(dst, 0, sizeof(*dst));

  const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(bbram));
  for (size_t i = 0; i < sizeof(*dst) / 4; i++) {
    bbram_read(dev, 4 * i, 4, &dst->data[4 * i]);
  }
}

void memfault_reboot_tracking_save(const sMemfaultRebootTrackingStorage *src) {
  // now back up reboot tracking to RTC backup regs
  const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(bbram));
  for (size_t i = 0; i < sizeof(*src) / 4; i++) {
    bbram_write(dev, 4 * i, 4, &src->data[4 * i]);
  }
}
#endif  // defined(CONFIG_BBRAM)

static int prv_log_overflow(const struct shell *shell, size_t argc, char **argv) {
  // get log line count from command line
  int log_line_count = 0;
  if (argc > 1) {
    log_line_count = strtoul(argv[1], NULL, 0);
  }

  for (int i = 0; i < log_line_count; i++) {
    LOG_INF("Log line %d", i);
  }

  shell_print(shell, "Dropped/recorded log lines: %d/%d", memfault_log_get_dropped_count(),
              memfault_log_get_recorded_count());

  return 0;
}

SHELL_CMD_REGISTER(log_overflow, NULL, "generate a number of log lines", prv_log_overflow);

int main(void) {
  LOG_INF("👋 Memfault Demo App! Board %s\n", CONFIG_BOARD);
  memfault_device_info_dump();

  memfault_cdr_register_source(&g_custom_data_recording_source);

#if !defined(CONFIG_MEMFAULT_RECORD_REBOOT_ON_SYSTEM_INIT)
  memfault_zephyr_collect_reset_info();
#endif

#if CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_METRICS
  s_main_thread = k_current_get();

  // @warning This code uses `memfault_metrics_heartbeat_debug_trigger` which is not intended
  // to be used in production code. This functions use here is solely to demonstrate the metrics
  // values changing. Production applications should rely on the heartbeat timer to trigger
  // collection

  // Collect a round of metrics to show initial stack usage
  memfault_metrics_heartbeat_debug_trigger();

  prv_run_stack_metrics_example();

  // Collect another round to show change in stack metrics
  memfault_metrics_heartbeat_debug_trigger();
#endif

  prv_init_test_thread_timer();

  blink_forever();

  return 0;
}
