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
  #include "memfault/core/platform/core.h"
  #include "memfault/metrics/battery.h"
  #include "memfault/metrics/metrics.h"
  #include "memfault/metrics/platform/battery.h"

static struct {
  //! stores the SOC at the time of the previous data collection. -1 means it
  //! was invalid
  int32_t last_state_of_charge;
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

static bool prv_soc_is_valid(int32_t soc) {
  return (soc >= 0);
}

//! Called as part of the end-of-heartbeat-interval collection functions
void memfault_metrics_battery_collect_data(void) {
  // cache the stopped_discharging value
  const bool stopped_discharging_during_interval =
    s_memfault_battery_ctx.stopped_discharging_during_interval;

  // now get the current discharging state, which is used to reset the
  // stopped_discharging context
  sMfltPlatformBatterySoc soc;
  int rv;
  bool is_currently_discharging;
  {
    rv = memfault_platform_get_stateofcharge(&soc);
    is_currently_discharging = soc.discharging;
    // Reset the stopped-discharging event tracking. Note that there's a race
    // condition here, if the memfault_metrics_battery_stopped_discharging()
    // function is called after the above memfault_platform_get_stateofcharge()
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

  // get the current SOC. set it as -1 if it's invalid.
  const int32_t current_stateofcharge = (rv == 0) ? ((int32_t)soc.soc) : (-1);

  const bool state_of_charge_valid = prv_soc_is_valid(current_stateofcharge) &&
                                     prv_soc_is_valid(s_memfault_battery_ctx.last_state_of_charge);
  if (!state_of_charge_valid) {
    MEMFAULT_LOG_WARN("Current (%d) or prev (%d) SOC invalid, dropping data",
                      (int)current_stateofcharge, (int)s_memfault_battery_ctx.last_state_of_charge);
  }

  // this means we didn't receive the stopped-discharging event as expected
  if (!is_currently_discharging && !stopped_discharging_during_interval) {
    MEMFAULT_LOG_WARN("Stopped-discharging event missed, dropping data");
  }

  // the session should be dropped if at any point during the session (including
  // right now) we weren't discharging
  const bool session_valid =
    is_currently_discharging && !stopped_discharging_during_interval && state_of_charge_valid;

  // accumulate SOC drop
  prv_accumulate_discharge_session_drop(session_valid, (uint32_t)current_stateofcharge);
  // save the current SOC
  s_memfault_battery_ctx.last_state_of_charge = current_stateofcharge;

  // record instantaneous SOC for single-device history
  if (prv_soc_is_valid(current_stateofcharge)) {
    MEMFAULT_METRIC_SET_UNSIGNED(battery_soc_pct, (uint32_t)current_stateofcharge);
  }
}

void memfault_metrics_battery_boot(void) {
  sMfltPlatformBatterySoc soc;
  int rv = memfault_platform_get_stateofcharge(&soc);
  // save starting SOC
  s_memfault_battery_ctx.last_state_of_charge = (rv == 0) ? ((int32_t)soc.soc) : (-1);
  // if the device is not discharging when it boots, save that condition
  s_memfault_battery_ctx.stopped_discharging_during_interval = !soc.discharging;
  s_memfault_battery_ctx.start_time_ms = memfault_platform_get_time_since_boot_ms();
}

#endif  // MEMFAULT_METRICS_BATTERY_ENABLE
