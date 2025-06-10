#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//! @brief APIs for tracking reliability metrics

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  // Time since boot at the start of the last heartbeat interval
  uint32_t last_heartbeat_ms;
  // Aggregated operational milliseconds since the last heartbeat serialization
  uint32_t operational_ms;
  bool counted_unexpected_reboot;
} sMemfaultMetricsReliabilityCtx;

//! Load reliability metrics state on boot. Called internally by the Memfault
//! SDK.
void memfault_metrics_reliability_boot(sMemfaultMetricsReliabilityCtx *ctx);

//! Returns the current reliability metrics context. Used to store it when
//! saving state on deep sleep or reboot.
sMemfaultMetricsReliabilityCtx *memfault_metrics_reliability_get_ctx(void);

//! Collects reliability metrics. This is normally called automatically during
//! the end of heartbeat timer callback, and should not be called by the user.
void memfault_metrics_reliability_collect(void);

#ifdef __cplusplus
}
#endif
