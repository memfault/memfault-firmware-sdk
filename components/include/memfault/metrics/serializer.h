#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Heartbeat metrics are collected at a periodic interval
//! (MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS). Each time they are collected, the data is
//! serialized out into a compressed format to be sent up to the Memfault cloud via the "Data
//! Packetizer" (see data_packetizer.h). The utilities in this module deal with this serialization
//! process

#include <stdbool.h>
#include <stddef.h>

#include "memfault/core/event_storage.h"
#include "memfault/metrics/ids_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Compute the worst case number of bytes required to serialize Memfault metrics for a heartbeat.
//!
//! @return the worst case amount of space needed to serialize an event
size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void);

//! Compute the worst case number of bytes required to serialize Memfault metrics for a given
//! session
size_t memfault_metrics_session_compute_worst_case_storage_size(eMfltMetricsSessionIndex session);

//! Serialize out the current set of heartbeat metrics
//!
//! @return True if the data was successfully serialized, else false if there was not enough space
//! to serialize the data
bool memfault_metrics_heartbeat_serialize(const sMemfaultEventStorageImpl *storage_impl);

bool memfault_metrics_session_serialize(const sMemfaultEventStorageImpl *storage_impl,
                                        eMfltMetricsSessionIndex session);

#ifdef __cplusplus
}
#endif
