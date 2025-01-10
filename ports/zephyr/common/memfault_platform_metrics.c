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

#if defined(CONFIG_MEMFAULT_METRICS_WIFI)
#include MEMFAULT_ZEPHYR_INCLUDE(net/net_mgmt.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/wifi_mgmt.h)
#endif

#include <stdbool.h>

#include "memfault/core/debug_log.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/zephyr/version.h"
#include "memfault/ports/zephyr/thread_metrics.h"

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

static MemfaultPlatformTimerCallback *s_metrics_timer_callback;

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
//! _system_heap is the heap used by k_malloc/k_free
extern struct sys_heap _system_heap;
#endif  // defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)

#if CONFIG_THREAD_RUNTIME_STATS
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

#if defined(CONFIG_MEMFAULT_METRICS_WIFI)
static void prv_record_iface_ap_oui(struct net_if *iface) {
  struct wifi_iface_status status = { 0 };

  if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status))) {
    return;
  }

  char oui[9];
  snprintf(oui, sizeof(oui), "%02x:%02x:%02x", status.bssid[0], status.bssid[1], status.bssid[2]);
  MEMFAULT_METRIC_SET_STRING(wifi_ap_oui, oui);
}

static void prv_wifi_event_callback(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
                                    struct net_if *iface) {
  switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: {
      const struct wifi_status *status = (const struct wifi_status *)cb->info;
      if (status->status) {
        // Future: Record connect errors
      } else {
        // start connected-time timer
        MEMFAULT_METRIC_TIMER_START(wifi_connected_time_ms);

        // record AP OUI
        prv_record_iface_ap_oui(iface);
      }
    } break;

    case NET_EVENT_WIFI_DISCONNECT_RESULT: {
      const struct wifi_status *status = (const struct wifi_status *)cb->info;
      if (status->status) {
        // TODO: Record disconnect error
      } else {
        // stop connected-time timer, increment disconnect count
        MEMFAULT_METRIC_TIMER_STOP(wifi_connected_time_ms);
        MEMFAULT_METRIC_ADD(wifi_disconnect_count, 1);
      }
    } break;

    default:
      // shouldn't happen since this callback only subscribes to the above events
      break;
  }
}

static void prv_init_wifi_metrics(void) {
  static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
  net_mgmt_init_event_callback(&wifi_shell_mgmt_cb, prv_wifi_event_callback,
                               // TODO: NET_EVENT_L4_CONNECTED + NET_EVENT_L4_DISCONNECTED instead?
                               NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);

  net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);
}

#endif  // defined(CONFIG_MEMFAULT_METRICS_WIFI)

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
static void prv_collect_cpu_temp(void) {
  struct sensor_value val;

  const struct device *dev = DEVICE_DT_GET(DT_ALIAS(die_temp0));

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

// Written as a function vs. in-line b/c we might want to extern this at some point?
// See ports/zephyr/config/memfault_metrics_heartbeat_zephyr_port_config.def for
// where the metrics key names come from.
void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE

  #if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
  struct k_thread *me = k_current_get();

    #if defined(CONFIG_THREAD_STACK_INFO)
  size_t free_stack_size;
  k_thread_stack_space_get(me, &free_stack_size);
  MEMFAULT_METRIC_SET_UNSIGNED(TimerTaskFreeStack, free_stack_size);
    #endif  // CONFIG_THREAD_STACK_INFO

    #if defined(CONFIG_THREAD_RUNTIME_STATS)
  static uint64_t s_prev_timer_task_cycles = 0;
  static uint64_t s_prev_all_tasks_cycles = 0;
  static uint64_t s_prev_non_idle_tasks_cycles = 0;

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
  uint64_t non_idle_tasks_cycles_delta =
    prv_cycle_delta_update(all_tasks_stats.total_cycles, &s_prev_non_idle_tasks_cycles);
  MEMFAULT_LOG_DEBUG("Non-idle tasks cycles: %u", (uint32_t)non_idle_tasks_cycles_delta);

  // Range is 0-10000 for 0.00-100.00%
  // Multiply by 100 for 2 decimals of precision via the scale factor, then by 100 again
  // for percentage conversion
  uint32_t usage_pct = (uint32_t)(non_idle_tasks_cycles_delta * 10000 / all_tasks_cycles_delta);
  MEMFAULT_METRIC_SET_UNSIGNED(cpu_usage_pct, usage_pct);
  MEMFAULT_LOG_DEBUG("CPU usage: %u.%02u%%\n", usage_pct / 100, usage_pct % 100);
      #endif  // MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 0)
    #endif    // CONFIG_THREAD_RUNTIME_STATS

  #endif /* defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) */

  #if CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC
  {
    struct fs_statvfs fs_stats;
    int retval = fs_statvfs("/" CONFIG_MEMFAULT_FS_BYTES_FREE_VFS_PATH, &fs_stats);
    if (retval == 0) {
      // compute free bytes
      uint32_t bytes_free = fs_stats.f_frsize * fs_stats.f_bfree;
      MEMFAULT_METRIC_SET_UNSIGNED(FileSystem_BytesFree, bytes_free);
    }
  }
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
}

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
#if defined(CONFIG_MEMFAULT_METRICS_WIFI)
  prv_init_wifi_metrics();
#endif

  s_metrics_timer_callback = callback;

  k_timer_start(&s_metrics_timer, K_SECONDS(period_sec), K_SECONDS(period_sec));
  return true;
}
