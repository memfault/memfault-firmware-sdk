#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @brief APIs for tracking reliability metrics

#ifdef __cplusplus
extern "C" {
#endif

//! Collects reliability metrics. This is normally called automatically during
//! the end of heartbeat timer callback, and should not be called by the user.
void memfault_metrics_reliability_collect(void);

#ifdef __cplusplus
}
#endif
