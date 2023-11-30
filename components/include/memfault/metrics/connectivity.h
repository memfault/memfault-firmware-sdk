#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief Connectivity metrics implementation.
//!
//! This module contains the implementation of the built in connectivity metrics
//! API. These metric pairs are available for use:
//!
//! - sync_successful/sync_failure: These metrics are used to count "sync"
//!   operation successes and failures, which Memfault will use to report sync
//!   success rate. A "sync" has an implementation-specific meaning.
//!
//! - sync_memfault_successful/sync_memfault_failure: These metrics are used to
//!   track syncs of data to Memfault. Platform ports like Zephyr and ESP-IDF
//!   may implement this metric by default.
//!
//! - connectivity_connected_time_ms/expected_connected_time_ms: These metrics
//!   are used for tracking "connection uptime" rate, for devices that are
//!   expected to have a continuous connection. "Connection" is device-specific.
//!
//! All metrics are implemented as heartbeat metrics, and are opt-in (disabled
//! by default). Use the following flags in memfault_platform_config.h to enable
//! the metrics:
//!
//! - MEMFAULT_METRICS_SYNC_SUCCESS
//! - MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS
//! - MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME

#include "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MEMFAULT_METRICS_SYNC_SUCCESS
//! Record a successful or failed sync event.
//!
//! This API can be used to count "sync" successes and failures. A "sync" has an
//! implementation-specific meaning. Some examples of sync events:
//!
//! - transferring data over BLE
//! - uploading data over MQTT
//! - sending data over a cellular modem
//!
//! Using this standard metric will enable some automatic "sync reliability"
//! analysis by the Memfault backend.
void memfault_metrics_connectivity_record_sync_success(void);
void memfault_metrics_connectivity_record_sync_failure(void);
#endif  // MEMFAULT_METRICS_SYNC_SUCCESS

#if MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS
//! Record a successful or failed Memfault sync event.
//!
//! This metric measures a specific sync type, "Memfault syncs", which are
//! uploads of data to Memfault itself.
void memfault_metrics_connectivity_record_memfault_sync_success(void);
void memfault_metrics_connectivity_record_memfault_sync_failure(void);
#endif  // MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS

#if MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME
//! Connectivity states:
//! | State        | Should be connected? | Is connected? |
//! |--------------|----------------------|---------------|
//! | Stopped      | No                   | No            |
//! | Started      | Yes                  | No            |
//! | Connected    | Yes                  | Yes           |
//! | Disconnected | Yes                  | No            |
typedef enum {
  kMemfaultMetricsConnectivityState_Stopped,
  kMemfaultMetricsConnectivityState_Started,
  kMemfaultMetricsConnectivityState_Connected,
  kMemfaultMetricsConnectivityState_ConnectionLost,
} eMemfaultMetricsConnectivityState;

//! Signal a connectivity state change event. For a device that should be always
//! connected, this would be a typical example state sequence:
//!
//! - Started (device boots, and starts attempting to connect)
//! - Connected (device connects)
//! - Connection Lost (some problem, eg wifi AP out of range; device disconnects)
//! - Connected (device reconnects)
//!
//! This API is thread-safe for supported platforms, but is not interrupt-safe:
//! it uses the Memfault Metrics Timer API, which is not interrupt-safe on most
//! platforms.
//!
//! @param state The new connectivity state.
void memfault_metrics_connectivity_connected_state_change(eMemfaultMetricsConnectivityState state);
#endif  // MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME

#ifdef __cplusplus
}
#endif
