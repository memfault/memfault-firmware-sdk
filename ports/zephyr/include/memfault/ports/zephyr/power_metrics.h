#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#ifdef __cplusplus
extern "C" {
#endif

//! Called during heartbeat collection to report accumulated time-in-PM-state
//! metrics and reset the underlying accumulators for the next window. See
//! memfault_platform_power_metrics.c for how the accumulation itself works.
void memfault_zephyr_power_metrics_collect(void);

#ifdef __cplusplus
}
#endif
