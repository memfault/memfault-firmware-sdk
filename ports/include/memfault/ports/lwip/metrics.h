//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus

//! Collects a round TCP/UDP metrics from LwIP
void memfault_lwip_heartbeat_collect_data(void);

#ifdef __cplusplus
}
#endif  // _cplusplus
