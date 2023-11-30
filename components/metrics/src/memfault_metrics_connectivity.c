//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief Connectivity metrics implementation.

#include "memfault/metrics/connectivity.h"

// non-module includes below

#include "memfault/core/debug_log.h"
#include "memfault/metrics/metrics.h"

// Sync success/failure metrics. The metrics add return code is ignored, this
// should not fail, but there's no action to take if it does.

#if MEMFAULT_METRICS_SYNC_SUCCESS
void memfault_metrics_connectivity_record_sync_success(void) {
  (void)MEMFAULT_METRIC_ADD(sync_successful, 1);
}

void memfault_metrics_connectivity_record_sync_failure(void) {
  (void)MEMFAULT_METRIC_ADD(sync_failure, 1);
}
#endif  // MEMFAULT_METRICS_SYNC_SUCCESS

#if MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS
void memfault_metrics_connectivity_record_memfault_sync_success(void) {
  (void)MEMFAULT_METRIC_ADD(sync_memfault_successful, 1);
}

void memfault_metrics_connectivity_record_memfault_sync_failure(void) {
  (void)MEMFAULT_METRIC_ADD(sync_memfault_failure, 1);
}
#endif  // MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS

#if MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME
void memfault_metrics_connectivity_connected_state_change(eMemfaultMetricsConnectivityState state) {
  switch (state) {
    case kMemfaultMetricsConnectivityState_Stopped:
      (void)MEMFAULT_METRIC_TIMER_STOP(connectivity_connected_time_ms);
      (void)MEMFAULT_METRIC_TIMER_STOP(connectivity_expected_time_ms);
      break;
    case kMemfaultMetricsConnectivityState_Started:
      (void)MEMFAULT_METRIC_TIMER_START(connectivity_expected_time_ms);
      break;
    case kMemfaultMetricsConnectivityState_Connected:
      // In case the "Started" state was skipped, start the expected time timer
      // here as well
      (void)MEMFAULT_METRIC_TIMER_START(connectivity_expected_time_ms);
      (void)MEMFAULT_METRIC_TIMER_START(connectivity_connected_time_ms);
      break;
    case kMemfaultMetricsConnectivityState_ConnectionLost:
      // In case the "Started" state was skipped, start the expected time timer
      // here as well. "ConnectionLost" means the connection SHOULD be up, but
      // isn't
      (void)MEMFAULT_METRIC_TIMER_START(connectivity_expected_time_ms);
      (void)MEMFAULT_METRIC_TIMER_STOP(connectivity_connected_time_ms);
      break;
    default:
      MEMFAULT_LOG_ERROR("Unexpected connection state: %u", state);
      break;
  }
}
#endif  // MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME
