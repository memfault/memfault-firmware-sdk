#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Required dependency functions for the Memfault Battery Metrics component.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MfltPlatformBatterySoc {
  // Battery state of charge, scaled by MEMFAULT_METRICS_BATTERY_SOC_PCT_SCALE_VALUE (default: 1).
  // Valid range: 0 to (100 * scale value).
  //
  // Example mappings:
  // +------------+-------------+------------+-------------------------------+
  // | Battery %  | Scale Value | .soc Value | battery_soc_pct in Memfault   |
  // +------------+-------------+------------+-------------------------------+
  // | 85%        | 1           | 85         | 85                            |
  // | 85.3%      | 10          | 853        | 85.3                          |
  // | 85.37%     | 100         | 8537       | 85.37                         |
  // +------------+-------------+------------+-------------------------------+
  uint32_t soc;
  // Whether the battery is currently discharging.
  bool discharging;
} sMfltPlatformBatterySoc;

//! The platform must implement this function. It provides the current battery
//! state of charge, and the discharging state.
//!
//! Certain device conditions may prevent reading a valid state of charge. In
//! that case, the function should return a non-zero value, and the state of
//! charge will be ignored. The discharging state will still be used and must
//! always be valid.
//!
//! @param soc A pointer to a sMfltPlatformBatterySoc struct to be populated
//!
//! @return 0 if SoC is successfully resolved, non-zero otherwise.
int memfault_platform_get_stateofcharge(sMfltPlatformBatterySoc *soc);

#ifdef __cplusplus
}
#endif
