//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault Battery Metrics implementation. See header file for details.

#include "memfault/config.h"

#if MEMFAULT_METRICS_BATTERY_ENABLE

  #include "memfault/core/debug_log.h"
  #include "memfault/core/platform/battery.h"
  #include "memfault/core/platform/core.h"
  #include "memfault/metrics/battery.h"
  #include "memfault/metrics/metrics.h"

static struct {
  //! stores the SOC at the time of the previous data collection
  uint32_t last_state_of_charge;
  //! start time of the current interval
  uint64_t start_time_ms;
  //! since the last data collection, did discharging stop?
  bool stopped_discharging_during_interval;
} s_memfault_battery_ctx;

static void prv_accumulate_discharge_session_drop(bool session_valid,
                                                  uint32_t current_stateofcharge) {
  const int32_t drop =
    (int32_t)s_memfault_battery_ctx.last_state_of_charge - (int32_t)current_stateofcharge;

  if (drop < 0) {
    MEMFAULT_LOG_DEBUG("Battery drop was negative (%" PRIi32 "), skipping", drop);
  }

  // compute elapsed time since last data collection
  const uint64_t current_time = memfault_platform_get_time_since_boot_ms();
  const uint32_t session_length_ms = current_time - s_memfault_battery_ctx.start_time_ms;
  s_memfault_battery_ctx.start_time_ms = current_time;

  if (session_valid && (drop >= 0)) {
    // explicitly write a 0 drop if there was a 0% SOC change (">=0") during the
    // discharge, instead of leaving it unset, which would result in CBOR null
    MEMFAULT_METRIC_SET_UNSIGNED(battery_soc_pct_drop, (uint32_t)drop);

    // store the session length
    MEMFAULT_METRIC_SET_UNSIGNED(battery_discharge_duration_ms, session_length_ms);
  }
}

void memfault_metrics_battery_stopped_discharging(void) {
  s_memfault_battery_ctx.stopped_discharging_during_interval = true;
}

//! Called as part of the end-of-heartbeat-interval collection functions
void memfault_metrics_battery_collect_data(void) {
  // cache the stopped_discharging value
  const bool stopped_discharging_during_interval =
    s_memfault_battery_ctx.stopped_discharging_during_interval;

  // now get the current discharging state, which is used to reset the
  // stopped_discharging context
  bool is_currently_discharging;
  {
    is_currently_discharging = memfault_platform_is_discharging();
    // Reset the stopped-discharging event tracking. Note that there's a race
    // condition here, if the memfault_metrics_battery_stopped_discharging()
    // function is called after the above memfault_platform_is_discharging()
    // call but before the below STR instruction.
    //
    // This can result in invalid SOC drop data if the charger is disconnected
    // before the end of session, because the device was not actually
    // discharging throughout the whole session.
    //
    // Ideally this block would be enclosed in a critical section (or atomic
    // get-and-set), but that's challenging to support across all our platforms,
    // so just accept that in extremely rare occasions, a bad session may be
    // recorded.
    s_memfault_battery_ctx.stopped_discharging_during_interval = !is_currently_discharging;
  }

  const uint32_t current_stateofcharge = memfault_platform_get_stateofcharge();

  // this means we didn't receive the stopped-discharging event as expected
  if (!is_currently_discharging && !stopped_discharging_during_interval) {
    MEMFAULT_LOG_WARN("Battery stopped-discharging event missed! dropping session discharge data");
  }

  // the session should be dropped if at any point during the session (including
  // right now) we weren't discharging
  const bool session_valid = is_currently_discharging && !stopped_discharging_during_interval;

  // accumulate SOC drop
  prv_accumulate_discharge_session_drop(session_valid, current_stateofcharge);
  // save the current SOC
  s_memfault_battery_ctx.last_state_of_charge = current_stateofcharge;

  // always record instantaneous SOC for single-device history
  MEMFAULT_METRIC_SET_UNSIGNED(battery_soc_pct, (uint32_t)current_stateofcharge);
}

void memfault_metrics_battery_boot(void) {
  // save starting SOC
  s_memfault_battery_ctx.last_state_of_charge = memfault_platform_get_stateofcharge();
  // if the device is not discharging when it boots, save that condition
  s_memfault_battery_ctx.stopped_discharging_during_interval = !memfault_platform_is_discharging();
  s_memfault_battery_ctx.start_time_ms = memfault_platform_get_time_since_boot_ms();
}

#endif  // MEMFAULT_METRICS_BATTERY_ENABLE
