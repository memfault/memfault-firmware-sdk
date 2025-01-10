//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief

#include <stdint.h>

#include "memfault/metrics/metrics.h"
#include "memfault/ports/freertos/metrics.h"
#include "memfault/ports/freertos/thread_metrics.h"

#if !defined(MEMFAULT_METRICS_HEARTBEAT_FREERTOS_CONFIG_DEF)
  #error \
    "Please include memfault_metrics_heartbeat_freertos_config.def in your memfault_metrics_heartbeat_config.def file"
#endif

#ifdef MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #include "esp_idf_version.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "freertos/timers.h"
#else
  #include "FreeRTOS.h"
  #include "task.h"
  #include "timers.h"
#endif

#if MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS
  // Older versions of FreeRTOS do not have this type, default to uint32_t
  #ifndef configRUN_TIME_COUNTER_TYPE
    #define configRUN_TIME_COUNTER_TYPE uint32_t
  #endif

  // Default to the original API names. FreeRTOS defaults to
  // configENABLE_BACKWARD_COMPATIBILITY=1, so newer versions are more likely to
  // support the original API.
  #if !defined(MEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API)
    #define MEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API 0
  #endif

  // Defines the precision used to calculate changes in cpu_usage_pct
  // Changes in this value require changing the scale_value used with cpu_usage_pct metrics
  // according to this equation: 100 = PRECISION_CONSTANT / scale_value
  // See ports/freertos/config/memfault_metrics_heartbeat_freertos_config.def
  #define PRECISION_CONSTANT (10000ULL)
  // A fudge factor used to improve rounding behavior at low/high end
  #define FUDGE_FACTOR (PRECISION_CONSTANT / 2ULL)

static configRUN_TIME_COUNTER_TYPE prv_calculate_delta_runtime(
  configRUN_TIME_COUNTER_TYPE prev_runtime_ticks, configRUN_TIME_COUNTER_TYPE curr_runtime_ticks) {
  return curr_runtime_ticks - prev_runtime_ticks;
}

// NB: We calculate a percentage that may need to be scaled according to PRECISION constant:
// - 1 percent: 1000 ticks/s * 60 s/min * 60 min/hr * (1/100)hrs -> 36,000 ticks
// - 1 permyriad: 1000 ticks/s * 60 s/min * 60 min/hr * (1/10000)hrs -> 360 ticks
// The PRECISION_CONSTANT allows handling higher tick rates at higher values (i.e. percent vs
// permyriad)
static int32_t prv_calc_runtime_percent(configRUN_TIME_COUNTER_TYPE prev_task_runtime,
                                        configRUN_TIME_COUNTER_TYPE curr_task_runtime,
                                        configRUN_TIME_COUNTER_TYPE curr_total_runtime,
                                        configRUN_TIME_COUNTER_TYPE prev_total_runtime) {
  uint64_t delta_runtime = prv_calculate_delta_runtime(prev_task_runtime, curr_task_runtime);
  uint64_t delta_total_runtime =
    prv_calculate_delta_runtime(prev_total_runtime, curr_total_runtime);

  // Guard against divide by zero
  if (delta_total_runtime == 0) {
    return -1;
  }

  // Make sure to not overflow when computing the percentage below
  if (delta_runtime >= UINT64_MAX - (UINT64_MAX / PRECISION_CONSTANT)) {
    // Scale both parts of the fraction by 10000 to avoid overflow.
    // Include a fudge factor of 5000 to prevent rounding errors.
    delta_runtime =
      ((UINT64_MAX - FUDGE_FACTOR) < delta_runtime) ? (UINT64_MAX) : (delta_runtime + FUDGE_FACTOR);
    delta_runtime /= PRECISION_CONSTANT;
    delta_total_runtime /= PRECISION_CONSTANT;
  }

  // Clamp delta_runtime to be less than delta_total_runtime
  // This can happen due to differences in when the total runtime vs a task's runtime are updated
  if (delta_runtime > delta_total_runtime) {
    delta_runtime = delta_total_runtime;
  }

  // Now compute percentage
  const int32_t percentage = (int32_t)((delta_runtime * PRECISION_CONSTANT) / delta_total_runtime);
  return percentage;
}

static configRUN_TIME_COUNTER_TYPE prv_get_total_runtime(void) {
  configRUN_TIME_COUNTER_TYPE total_runtime;
  #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
  portALT_GET_RUN_TIME_COUNTER_VALUE(total_runtime);
  #else
  total_runtime = portGET_RUN_TIME_COUNTER_VALUE();
  #endif
  return total_runtime;
}
#endif

#if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
static configRUN_TIME_COUNTER_TYPE prv_get_idle_counter_for_core(uint32_t core) {
  TaskHandle_t idle_task_handle =
  #ifdef MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    xTaskGetIdleTaskHandleForCore(core)
    #else
    xTaskGetIdleTaskHandleForCPU(core)
    #endif
  #else
    xTaskGetIdleTaskHandleForCPU(core)
  #endif
    ;
  TaskStatus_t idle_task_status;

  // This could be a bit cleaner if we could use `ulTaskGetRunTimeCounter`, but that API
  // is not available in FreeRTOS < 10.4.4.
  vTaskGetInfo(idle_task_handle, &idle_task_status, pdTRUE, eRunning);
  return idle_task_status.ulRunTimeCounter;
}
#endif

#if MEMFAULT_FREERTOS_COLLECT_TIMER_STACK_FREE_BYTES
// FreeRTOS v9.0.0 made the xTimerGetTimerDaemonTaskHandle() API always
// available:
// https://github.com/FreeRTOS/FreeRTOS-Kernel/commit/6568ba6eb08a5ed0adf5141291d31f2144a79d08#diff-343560675a0f127a21d52dd3379ff23b01fe59fe87db61669efd3c903621fc22R427-R433
  #if (tskKERNEL_VERSION_MAJOR < 9) && (!INCLUDE_xTimerGetTimerDaemonTaskHandle)
    #error \
      "MEMFAULT_FREERTOS_COLLECT_TIMER_STACK_FREE_BYTES requires FreeRTOS v9.0.0 or later, or INCLUDE_xTimerGetTimerDaemonTaskHandle=1 in FreeRTOSConfig.h"
  #endif

static void prv_record_timer_stack_free_bytes(void) {
  TaskHandle_t timer_task_handle = xTimerGetTimerDaemonTaskHandle();

  UBaseType_t free_space = uxTaskGetStackHighWaterMark(timer_task_handle);

  // uxTaskGetStackHighWaterMark() returns units of "words", so convert to bytes
  MEMFAULT_METRIC_SET_UNSIGNED(timer_task_stack_free_bytes, free_space * sizeof(StackType_t));
}
#endif

void memfault_freertos_port_task_runtime_metrics(void) {
#if MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS
  static configRUN_TIME_COUNTER_TYPE s_prev_idle_runtime = 0;

  // Only collect separate cpu1 runtime if multi-core split
  #if MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT
  static configRUN_TIME_COUNTER_TYPE s_prev_idle1_runtime = 0;
  #endif

  static configRUN_TIME_COUNTER_TYPE s_prev_total_runtime = 0;

  configRUN_TIME_COUNTER_TYPE idle_runtime =
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
    prv_get_idle_counter_for_core(0);
  #else
    #if MEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API
    ulTaskGetIdleRunTimeCounter
    #else
    xTaskGetIdleRunTimeCounter
    #endif
    ();
  #endif

  #if MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT
  configRUN_TIME_COUNTER_TYPE idle1_runtime = prv_get_idle_counter_for_core(1);
  #endif

  configRUN_TIME_COUNTER_TYPE total_runtime = prv_get_total_runtime();

  int32_t idle_runtime_percent = prv_calc_runtime_percent(s_prev_idle_runtime, idle_runtime,
                                                          total_runtime, s_prev_total_runtime);
  #if MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT
  int32_t idle1_runtime_percent = prv_calc_runtime_percent(s_prev_idle1_runtime, idle1_runtime,
                                                           total_runtime, s_prev_total_runtime);
  #endif

  s_prev_idle_runtime = idle_runtime;
  #if MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT
  s_prev_idle1_runtime = idle1_runtime;
  #endif
  s_prev_total_runtime = total_runtime;

  if (idle_runtime_percent >= 0) {
    MEMFAULT_METRIC_SET_UNSIGNED(cpu_usage_pct,
                                 (uint32_t)(PRECISION_CONSTANT - (uint32_t)idle_runtime_percent));
  }

  #if MEMFAULT_FREERTOS_RUNTIME_STATS_MULTI_CORE_SPLIT
  if (idle1_runtime_percent >= 0) {
    MEMFAULT_METRIC_SET_UNSIGNED(cpu1_usage_pct,
                                 (uint32_t)(PRECISION_CONSTANT - (uint32_t)idle1_runtime_percent));
  }
  #endif
#endif  // MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS

#if MEMFAULT_FREERTOS_COLLECT_TIMER_STACK_FREE_BYTES
  prv_record_timer_stack_free_bytes();
#endif

#if MEMFAULT_FREERTOS_COLLECT_THREAD_METRICS
  memfault_freertos_port_thread_metrics();
#endif
}
