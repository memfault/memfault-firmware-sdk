#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Stores serialized event information that is ready to be sent up to the Memfault Cloud
//! (i.e Heartbeat Metrics & Reboot Reasons). Must be initialized on system boot.
//!
//! @note If calls to data_packetizer.c are made on a different task than the one
//! MemfaultPlatformTimerCb is invoked on, memfault_lock() & memfault_unlock() should also
//! be implemented by the platform

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemfaultEventStorageImpl sMemfaultEventStorageImpl;

//! Must be called by the customer on boot to setup heartbeat storage.
//!
//! This is where serialized heartbeat data is stored as it waits to be drained
//! (via call's to memfault_packetizer_get_next())
//!
//! For any module using the event store the worst case size needed can be computed using the
//! exported APIs:
//!   memfault_reboot_tracking_compute_worst_case_storage_size()
//!   memfault_metrics_heartbeat_compute_worst_case_storage_size()
//!
//! For example, if a device connects to the internet once/hour and collects a heartbeat at this
//! interval as well, the buffer size to allocate can easily be computed as:
//!  buf_len = memfault_metrics_heartbeat_compute_worst_case_storage_size() * 1
//!
//! When a module using the event store is initialized a a WARNING will print on boot and an error
//! code will return if it is not appropriately sized to hold at least one event.
//!
//! @param buf The buffer to use for heartbeat storage
//! @param buf_len The length of the buffer to use for heartbeat storage
//!
//! @return a handle to the event storage implementation on success & false on failure.
//!  This handle will need to be provided to modules which use the event store on initialization
const sMemfaultEventStorageImpl *memfault_events_storage_boot(void *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif
