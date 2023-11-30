//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include <stdbool.h>

#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/zephyr/version.h"

#if CONFIG_MEMFAULT_FS_BYTES_FREE_METRIC
#include MEMFAULT_ZEPHYR_INCLUDE(fs/fs.h)
#endif
// clang-format on

static MemfaultPlatformTimerCallback *s_metrics_timer_callback;

#if CONFIG_SYS_HEAP_RUNTIME_STATS
//! _system_heap is the heap used by k_malloc/k_free
extern struct sys_heap _system_heap;
#endif  /* CONFIG_SYS_HEAP_RUNTIME_STATS */

#if CONFIG_THREAD_RUNTIME_STATS
static void prv_execution_cycles_delta_update(MemfaultMetricId key, uint64_t curr_cycles,
                                              uint64_t *prev_cycles) {
  uint64_t heartbeat_cycles;
  if (*prev_cycles > curr_cycles) {
    heartbeat_cycles = UINT64_MAX - *prev_cycles + curr_cycles;
  } else {
    heartbeat_cycles = curr_cycles - *prev_cycles;
  }
  memfault_metrics_heartbeat_set_unsigned(key, heartbeat_cycles);
  *prev_cycles = curr_cycles;
}
#endif

// Written as a function vs. in-line b/c we might want to extern this at some point?
// See ports/zephyr/config/memfault_metrics_heartbeat_zephyr_port_config.def for
// where the metrics key names come from.
void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
  struct k_thread *me = k_current_get();

#if defined(CONFIG_THREAD_STACK_INFO) && MEMFAULT_ZEPHYR_VERSION_GT(2, 1)
  // k_thread_stack_space_get() introduced in v2.2.0
  size_t free_stack_size;
  k_thread_stack_space_get(me, &free_stack_size);
  MEMFAULT_METRIC_SET_UNSIGNED(TimerTaskFreeStack, free_stack_size);
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS)
  static uint64_t s_prev_thread_execution_cycles = 0;
  static uint64_t s_prev_all_execution_cycles = 0;

  k_thread_runtime_stats_t rt_stats_thread;
  k_thread_runtime_stats_get(me, &rt_stats_thread);
  prv_execution_cycles_delta_update(MEMFAULT_METRICS_KEY(TimerTaskCpuUsage),
                                    rt_stats_thread.execution_cycles,
                                    &s_prev_thread_execution_cycles);

  k_thread_runtime_stats_t rt_stats_all;
  k_thread_runtime_stats_all_get(&rt_stats_all);
  prv_execution_cycles_delta_update(MEMFAULT_METRICS_KEY(AllTasksCpuUsage),
                                    rt_stats_all.execution_cycles,
                                    &s_prev_all_execution_cycles);
#endif

#endif /* defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO) */

#if CONFIG_SYS_HEAP_RUNTIME_STATS

#if MEMFAULT_ZEPHYR_VERSION_GT(3, 1)
  // struct sys_memory_stats introduced in v3.2
  struct sys_memory_stats stats = {0};
#else
  struct sys_heap_runtime_stats stats = {0};
#endif

  sys_heap_runtime_stats_get(&_system_heap, &stats);
  MEMFAULT_METRIC_SET_UNSIGNED(Heap_BytesFree, stats.free_bytes);
#endif /* defined(CONFIG_SYS_HEAP_RUNTIME_STATS) */

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

bool memfault_platform_metrics_timer_boot(
    uint32_t period_sec, MemfaultPlatformTimerCallback callback) {
  s_metrics_timer_callback = callback;

  k_timer_start(&s_metrics_timer, K_SECONDS(period_sec), K_SECONDS(period_sec));
  return true;
}
