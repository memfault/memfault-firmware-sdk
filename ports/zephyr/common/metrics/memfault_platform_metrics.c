//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/sensor.h)
#endif

#include <stdbool.h>

#include "memfault/core/debug_log.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/zephyr/version.h"
#include "memfault/ports/zephyr/thread_metrics.h"

#if defined(CONFIG_MEMFAULT_METRICS_BLUETOOTH)
#include "memfault/ports/zephyr/bluetooth_metrics.h"
#endif

#if defined(CONFIG_MEMFAULT_METRICS_TCP_IP)
#include MEMFAULT_ZEPHYR_INCLUDE(net/net_stats.h)
// Directory traversal is needed to access the header for net_stats_reset(),
// which is not in the global Zephyr search path.
#include MEMFAULT_ZEPHYR_INCLUDE(../../subsys/net/ip/net_stats.h)
#endif
#if CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC
#include MEMFAULT_ZEPHYR_INCLUDE(fs/fs.h)
#endif
// clang-format on

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
//! _system_heap is the heap used by k_malloc/k_free
extern struct sys_heap _system_heap;
#endif  // defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)

#if defined(CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE)
static uint64_t prv_cycle_delta_update(uint64_t curr_cycles, uint64_t *prev_cycles) {
  // computes delta correctly if prev > curr due to overflow
  uint64_t delta = curr_cycles - *prev_cycles;
  *prev_cycles = curr_cycles;
  return delta;
}
#endif

#if defined(CONFIG_MEMFAULT_METRICS_TCP_IP)
static void prv_collect_ip_statistics(void) {
  struct net_stats data;

  int err = net_mgmt(NET_REQUEST_STATS_GET_ALL, NULL, &data, sizeof(data));
  if (err) {
    return;
  }

  // save the most interesting stats as metrics, and reset the stats structure
  #if defined(CONFIG_NET_STATISTICS_UDP)
  MEMFAULT_METRIC_SET_UNSIGNED(net_udp_recv, data.udp.recv);
  MEMFAULT_METRIC_SET_UNSIGNED(net_udp_sent, data.udp.sent);
  #endif
  #if defined(CONFIG_NET_STATISTICS_TCP)
  MEMFAULT_METRIC_SET_UNSIGNED(net_tcp_recv, data.tcp.recv);
  MEMFAULT_METRIC_SET_UNSIGNED(net_tcp_sent, data.tcp.sent);
  #endif

  MEMFAULT_METRIC_SET_UNSIGNED(net_bytes_received, data.bytes.received);
  MEMFAULT_METRIC_SET_UNSIGNED(net_bytes_sent, data.bytes.sent);

  // reset the stats
  net_stats_reset(NULL);
}
#endif  // defined(MEMFAULT_METRICS_TCP_IP)

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
static void prv_collect_cpu_temp(void) {
  struct sensor_value val;

  // Check for a compatible CPU temperature sensor node in the device tree

  // 1. Check for a user-defined alias first
  #if DT_NODE_HAS_STATUS(DT_ALIAS(memfault_cpu_temp), okay)
    #define CPU_TEMP_NODE_ID DT_ALIAS(memfault_cpu_temp)
  // 2. 'nordic_nrf_temp' is used on Nordic nRF51/nRF52/nRF53/nRF54 series
  #elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_temp)
    #define CPU_TEMP_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_temp)
  // 3. 'temp' is used on some Nordic platforms
  #elif DT_NODE_HAS_STATUS(DT_NODELABEL(temp), okay)
    #define CPU_TEMP_NODE_ID DT_NODELABEL(temp)
  // 4. 'die_temp0' and 'die_temp' are commonly used on STM32 and NXP platforms
  #elif DT_NODE_HAS_STATUS(DT_ALIAS(die_temp0), okay)
    #define CPU_TEMP_NODE_ID DT_ALIAS(die_temp0)
  #elif DT_NODE_HAS_STATUS(DT_ALIAS(die_temp), okay)
    #define CPU_TEMP_NODE_ID DT_ALIAS(die_temp)
  #else
    #error \
      "No CPU temperature sensor found, make sure to enable a compatible node in the device tree"
  #endif

  const struct device *dev = DEVICE_DT_GET(CPU_TEMP_NODE_ID);

  if (!device_is_ready(dev)) {
    return;
  }

  if (sensor_sample_fetch(dev)) {
    return;
  }

  if (sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val)) {
    return;
  }

  // val1 is the integer part and val2 is fractional millionths. Scale both to match metric
  // precision
  const int32_t temperature = (val.val1 * 10) + (val.val2 / 100000);

  MEMFAULT_LOG_DEBUG("CPU Temp: %d.%06d", val.val1, val.val2);

  MEMFAULT_METRIC_SET_SIGNED(thermal_cpu_c, temperature);
}
#endif  // defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
static void prv_collect_memory_usage_metrics(void) {
  #if MEMFAULT_ZEPHYR_VERSION_GT(3, 1)
  // struct sys_memory_stats introduced in v3.2
  struct sys_memory_stats stats = { 0 };
  #else
  struct sys_heap_runtime_stats stats = { 0 };
  #endif

  sys_heap_runtime_stats_get(&_system_heap, &stats);
  MEMFAULT_METRIC_SET_UNSIGNED(Heap_BytesFree, stats.free_bytes);

  #if MEMFAULT_ZEPHYR_VERSION_GT(3, 0)
  // stats.max_allocated_bytes was added in Zephyr 3.1
  //
  // Range is 0-10000 for 0.00-100.00%
  // Multiply by 100 for 2 decimals of precision via the scale factor, then by 100 again
  // for percentage conversion
  MEMFAULT_METRIC_SET_UNSIGNED(memory_pct_max, (uint32_t)(stats.max_allocated_bytes * 10000.f) /
                                                 (stats.free_bytes + stats.allocated_bytes));
  #endif  // MEMFAULT_ZEPHYR_VERSION_GT(3, 0)
}
#endif  // defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)

#if defined(CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC)
void prv_collect_fs_bytes_free_metric(void) {
  const char *mount_point = NULL;

  // User configured path takes priority
  #ifdef CONFIG_MEMFAULT_FS_BYTES_FREE_VFS_PATH
  char normalized_path[64];
  if (CONFIG_MEMFAULT_FS_BYTES_FREE_VFS_PATH[0] != '\0') {
    MEMFAULT_LOG_DEBUG("Using user-configured mount point for FS bytes free metric");
    snprintf(normalized_path, sizeof(normalized_path), "/%s",
             CONFIG_MEMFAULT_FS_BYTES_FREE_VFS_PATH);
    mount_point = normalized_path;
  }
  #endif

  // Auto-detect from fstab only
  #if DT_NODE_EXISTS(DT_PATH(fstab))
  MEMFAULT_LOG_DEBUG("Auto-detecting mount point from fstab for FS bytes free metric");

    // Create a ternary chain to find the first mount point available
    //
    // The return statement here will expand to something like:
    // return (DT_NODE_HAS_PROP(child1, mount_point) ? DT_PROP(child1, mount_point) :)
    //        (DT_NODE_HAS_PROP(child2, mount_point) ? DT_PROP(child2, mount_point) :)
    //        (DT_NODE_HAS_PROP(child3, mount_point) ? DT_PROP(child3, mount_point) :)
    //         NULL;
    #define FIND_FIRST_MOUNT(node_id) \
      DT_NODE_HAS_PROP(node_id, mount_point) ? DT_PROP(node_id, mount_point):

  mount_point = DT_FOREACH_CHILD_SEP(DT_PATH(fstab), FIND_FIRST_MOUNT, ) NULL;
  #endif

  if (mount_point == NULL) {
    MEMFAULT_LOG_WARN("No mount point configured - skipping FS bytes free metric");
    return;
  }

  MEMFAULT_LOG_DEBUG("Collecting FS bytes free metric for mount point %s", mount_point);

  struct fs_statvfs fs_stats;
  int retval = fs_statvfs(mount_point, &fs_stats);
  if (retval == 0) {
    // compute free bytes
    uint32_t bytes_free = fs_stats.f_frsize * fs_stats.f_bfree;
    MEMFAULT_METRIC_SET_UNSIGNED(FileSystem_BytesFree, bytes_free);
  }
}

#endif /* CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC */

// Written as a function vs. in-line b/c we might want to extern this at some point?
// See ports/zephyr/config/memfault_metrics_heartbeat_zephyr_port_config.def for
// where the metrics key names come from.
void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if defined(CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE)
  struct k_thread *me = k_current_get();

  size_t free_stack_size;
  k_thread_stack_space_get(me, &free_stack_size);
  MEMFAULT_METRIC_SET_UNSIGNED(TimerTaskFreeStack, free_stack_size);

  static uint64_t s_prev_timer_task_cycles = 0;
  static uint64_t s_prev_all_tasks_cycles = 0;

  k_thread_runtime_stats_t timer_task_stats;
  k_thread_runtime_stats_get(me, &timer_task_stats);

  uint64_t task_cycles_delta =
    prv_cycle_delta_update(timer_task_stats.execution_cycles, &s_prev_timer_task_cycles);
  MEMFAULT_METRIC_SET_UNSIGNED(TimerTaskCpuUsage, (uint32_t)task_cycles_delta);
  MEMFAULT_LOG_DEBUG("Timer task cycles: %u", (uint32_t)task_cycles_delta);

  k_thread_runtime_stats_t all_tasks_stats;
  k_thread_runtime_stats_all_get(&all_tasks_stats);

  uint64_t all_tasks_cycles_delta =
    prv_cycle_delta_update(all_tasks_stats.execution_cycles, &s_prev_all_tasks_cycles);
  MEMFAULT_METRIC_SET_UNSIGNED(AllTasksCpuUsage, (uint32_t)all_tasks_cycles_delta);
  MEMFAULT_LOG_DEBUG("All tasks cycles: %u", (uint32_t)all_tasks_cycles_delta);

  // stats.total_cycles added in Zephyr 3.0
  #if MEMFAULT_ZEPHYR_VERSION_GTE_STRICT(3, 0)
  static uint64_t s_prev_non_idle_tasks_cycles = 0;

  if (all_tasks_cycles_delta > 0) {
    uint64_t non_idle_tasks_cycles_delta =
      prv_cycle_delta_update(all_tasks_stats.total_cycles, &s_prev_non_idle_tasks_cycles);
    MEMFAULT_LOG_DEBUG("Non-idle tasks cycles: %u", (uint32_t)non_idle_tasks_cycles_delta);

    // Range is 0-10000 for 0.00-100.00%
    // Multiply by 100 for 2 decimals of precision via the scale factor, then by 100 again
    // for percentage conversion
    uint32_t usage_pct = (uint32_t)(non_idle_tasks_cycles_delta * 10000 / all_tasks_cycles_delta);
    MEMFAULT_METRIC_SET_UNSIGNED(cpu_usage_pct, usage_pct);
    MEMFAULT_LOG_DEBUG("CPU usage: %u.%02u%%", usage_pct / 100, usage_pct % 100);
  }
  #endif  // MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 0)

  #if defined(CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC)
  prv_collect_fs_bytes_free_metric();
  #endif /* CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC */

#endif /* CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE */

#if defined(CONFIG_MEMFAULT_METRICS_TCP_IP)
  prv_collect_ip_statistics();
#endif

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
  prv_collect_cpu_temp();
#endif

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
  prv_collect_memory_usage_metrics();
#endif

#if defined(CONFIG_MEMFAULT_METRICS_THREADS)
  memfault_zephyr_thread_metrics_record();
#endif

#if defined(CONFIG_MEMFAULT_METRICS_BLUETOOTH)
  memfault_bluetooth_metrics_heartbeat_update();
#endif
}

#if !defined(CONFIG_MEMFAULT_METRICS_TIMER_CUSTOM)
static MemfaultPlatformTimerCallback *s_metrics_timer_callback;

static void prv_metrics_work_handler(struct k_work *work) {
  if (s_metrics_timer_callback != NULL) {
    s_metrics_timer_callback();
  }
}

K_WORK_DEFINE(s_metrics_timer_work, prv_metrics_work_handler);

//! Timer handlers run from an ISR so we dispatch the heartbeat job to the worker task
static void prv_timer_expiry_handler(struct k_timer *dummy) {
  k_work_submit(&s_metrics_timer_work);
}

K_TIMER_DEFINE(s_metrics_timer, prv_timer_expiry_handler, NULL);

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  s_metrics_timer_callback = callback;

  k_timer_start(&s_metrics_timer, K_SECONDS(period_sec), K_SECONDS(period_sec));
  return true;
}
#endif  // !defined(CONFIG_MEMFAULT_METRICS_TIMER_CUSTOM)
