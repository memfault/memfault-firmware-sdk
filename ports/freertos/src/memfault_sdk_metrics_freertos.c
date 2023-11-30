//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief

#include <stdint.h>

#include "memfault/metrics/metrics.h"
#include "memfault/ports/freertos/metrics.h"

#ifdef MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#else
  #include "FreeRTOS.h"
  #include "task.h"
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

static configRUN_TIME_COUNTER_TYPE prv_calculate_delta_runtime(
  configRUN_TIME_COUNTER_TYPE prev_runtime_ticks, configRUN_TIME_COUNTER_TYPE curr_runtime_ticks) {
  return curr_runtime_ticks - prev_runtime_ticks;
}

static int32_t prv_calc_runtime_percentage(configRUN_TIME_COUNTER_TYPE prev_task_runtime,
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
  if (delta_runtime >= UINT64_MAX - (UINT64_MAX / 100)) {
    // Scale both parts of the fraction by 100 to avoid overflow.
    // Include a fudge factor of 50 to prevent rounding errors.
    delta_runtime = ((UINT64_MAX - 50) < delta_runtime) ? (UINT64_MAX) : (delta_runtime + 50);
    delta_runtime /= 100;
    delta_total_runtime /= 100;
  }

  // Now compute percentage
  const int32_t percentage = (int32_t)((delta_runtime * 100ULL) / delta_total_runtime);
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
  TaskHandle_t idle_task_handle = xTaskGetIdleTaskHandleForCPU(core);
  TaskStatus_t idle_task_status;

  // This could be a bit cleaner if we could use `ulTaskGetRunTimeCounter`, but that API
  // is not available in FreeRTOS < 10.4.4.
  vTaskGetInfo(idle_task_handle, &idle_task_status, pdTRUE, eRunning);
  return idle_task_status.ulRunTimeCounter;
}
#endif

void memfault_freertos_port_task_runtime_metrics(void) {
#if MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS
  static configRUN_TIME_COUNTER_TYPE s_prev_idle0_runtime = 0;
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
  static configRUN_TIME_COUNTER_TYPE s_prev_idle1_runtime = 0;
  #endif
  static configRUN_TIME_COUNTER_TYPE s_prev_total_runtime = 0;

  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
  configRUN_TIME_COUNTER_TYPE idle0_runtime = prv_get_idle_counter_for_core(0);
  configRUN_TIME_COUNTER_TYPE idle1_runtime = prv_get_idle_counter_for_core(1);
  #else
  configRUN_TIME_COUNTER_TYPE idle0_runtime =
    #if MEMFAULT_USE_NEW_FREERTOS_IDLETASK_RUNTIME_API
    ulTaskGetIdleRunTimeCounter
    #else
    xTaskGetIdleRunTimeCounter
    #endif
    ();
  #endif
  configRUN_TIME_COUNTER_TYPE total_runtime = prv_get_total_runtime();

  int32_t idle0_runtime_percentage = prv_calc_runtime_percentage(
    s_prev_idle0_runtime, idle0_runtime, total_runtime, s_prev_total_runtime);
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
  int32_t idle1_runtime_percentage = prv_calc_runtime_percentage(
    s_prev_idle1_runtime, idle1_runtime, total_runtime, s_prev_total_runtime);
  #endif

  s_prev_idle0_runtime = idle0_runtime;
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
  s_prev_idle1_runtime = idle1_runtime;
  #endif
  s_prev_total_runtime = total_runtime;

  if (idle0_runtime_percentage >= 0) {
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
    MEMFAULT_METRIC_SET_UNSIGNED(idle0_task_run_time_percent,
                                    (uint32_t)idle0_runtime_percentage);
  #else
    MEMFAULT_METRIC_SET_UNSIGNED(idle_task_run_time_percent, (uint32_t)idle0_runtime_percentage);
  #endif
  }
  #if MEMFAULT_FREERTOS_RUN_TIME_STATS_MULTI_CORE
  if (idle1_runtime_percentage >= 0) {
    MEMFAULT_METRIC_SET_UNSIGNED(idle1_task_run_time_percent,
                                    (uint32_t)idle1_runtime_percentage);
  }
  #endif
#endif  // MEMFAULT_FREERTOS_COLLECT_RUN_TIME_STATS
}
