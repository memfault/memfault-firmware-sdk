//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Tracks time spent in each Zephyr Power Management (PM) state as Memfault
//! metrics. This uses the generic zephyr/pm/pm.h notifier API, so it works
//! on any Zephyr port that implements the PM subsystem (pm_state_set()) -
//! it is not specific to any SoC vendor.
//!
//! Only PM_STATE_SUSPEND_TO_IDLE is tracked with substate granularity: it's
//! the one state that Zephyr SoC ports commonly split into multiple
//! substates (e.g. cache retained vs. disabled - see nRF92/nRF54H's
//! soc_power.c). The other states report a single aggregate metric each,
//! since a per-substate breakdown for them would almost always just be
//! zeros for everything but substate 0.
//!
//! IMPORTANT: per zephyr/pm/pm.h, PM notifier callbacks "can be called from
//! the ISR of the event that caused the kernel exit from idling." Memfault's
//! metrics APIs (e.g. the timer metric helpers) take a lock and must not be
//! called from ISR context - doing so trips
//! "ASSERTION FAIL [!arch_is_in_isr()] ... mutexes cannot be used inside
//! ISRs". So this module never calls into the metrics APIs from the
//! notifier callbacks themselves. Instead, it tallies elapsed ticks
//! locklessly into atomic_t accumulators from the callbacks, and only
//! reports/resets those values into real (unsigned) metrics later, from
//! thread context, when memfault_zephyr_power_metrics_collect() is called
//! during heartbeat collection.

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(pm/pm.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/atomic.h)
// clang-format on

#include "memfault/metrics/metrics.h"
#include "memfault/ports/zephyr/power_metrics.h"

//! Number of substate metric buckets tracked for a single PM state (the max
//! across all tracked states - only PM_STATE_SUSPEND_TO_IDLE uses more than
//! one).
//!
//! Substate IDs are SoC-defined (see the target's devicetree "power-states"
//! node); every known Zephyr port that implements substates for
//! PM_STATE_SUSPEND_TO_IDLE (e.g. nRF92, nRF54H) uses IDs 0-2. ID 0 is not
//! tracked (see MFLT_PM_METRICS_S2IDLE_FIRST_TRACKED_SUBSTATE_ID), leaving
//! IDs 1-2 with a bucket each. IDs at or beyond the last tracked ID fold
//! into the last bucket, so no time in a tracked substate goes untracked,
//! but those IDs won't be distinguishable from one another.
#define MFLT_PM_METRICS_MAX_SUBSTATES 2

//! Lowest PM_STATE_SUSPEND_TO_IDLE substate ID that gets a metric; time in
//! lower IDs is dropped, not folded into another bucket.
//!
//! Substate 0 ("idle, cache powered on") is not implemented by the SoC PM
//! code, so it is never actually entered and its metric would always read 0.
//! Substates 1 (cache retained) and 2 (cache disabled) are reachable,
//! depending on the target's configuration, so they keep their own metrics.
#define MFLT_PM_METRICS_S2IDLE_FIRST_TRACKED_SUBSTATE_ID 1

#if CONFIG_MP_MAX_NUM_CPUS > 1
  #include MEMFAULT_ZEPHYR_INCLUDE(arch/cpu.h)
#endif

//! Ticks accumulated in each (PM state, substate) pair since the last call
//! to memfault_zephyr_power_metrics_collect(). Only ever touched via
//! atomic_add() (from notifier callbacks, possibly in ISR context) and
//! atomic_set() (from collect(), in thread context), so no separate locking
//! is required.
//!
//! Only PM_STATE_SUSPEND_TO_IDLE uses more than one column; other tracked
//! states only ever use column 0. See prv_num_tracked_substates() for which
//! states are tracked at all and how many columns each one uses.
static atomic_t s_mflt_pm_state_ticks[PM_STATE_COUNT][MFLT_PM_METRICS_MAX_SUBSTATES];

//! Tick count at which the current CPU entered its current PM state.
//! Zephyr's PM core calls the entry/exit callbacks for a given CPU strictly
//! sequentially (entry, then exit, never nested or interleaved with another
//! entry for that same CPU), so plain per-CPU storage here is safe without
//! atomics.
static int64_t s_mflt_pm_state_entry_ticks[CONFIG_MP_MAX_NUM_CPUS];

//! Maps each tracked, non-active PM state to its Memfault metric key(s).
//!
//! Only row PM_STATE_SUSPEND_TO_IDLE has more than one meaningful column.
//! Its columns start at substate ID
//! MFLT_PM_METRICS_S2IDLE_FIRST_TRACKED_SUBSTATE_ID, not at 0 - see
//! prv_substate_index().
//!
//! The PM_STATE_ACTIVE row is left unset/unused: per the PM notifier
//! contract, the entry callback fires when leaving PM_STATE_ACTIVE for
//! another state, and the exit callback fires when returning to
//! PM_STATE_ACTIVE - in both cases the state argument passed is the
//! non-active state being entered/exited, never PM_STATE_ACTIVE itself.
static const MemfaultMetricId
  g_mflt_pm_state_metric_keys[PM_STATE_COUNT][MFLT_PM_METRICS_MAX_SUBSTATES] = {
    [PM_STATE_SUSPEND_TO_IDLE] = {
      MEMFAULT_METRICS_KEY(power_suspend_to_idle_s1_time_ms),
      MEMFAULT_METRICS_KEY(power_suspend_to_idle_s2_time_ms),
    },
    [PM_STATE_SUSPEND_TO_RAM] = { MEMFAULT_METRICS_KEY(power_suspend_to_ram_time_ms) },
};

//! Number of substate "columns" tracked for a given state. Bounds all
//! iteration/indexing into the arrays above, so unused columns are never
//! read or written.
static uint8_t prv_num_tracked_substates(enum pm_state state) {
  switch (state) {
    case PM_STATE_SUSPEND_TO_IDLE:
      return MFLT_PM_METRICS_MAX_SUBSTATES;
    case PM_STATE_SUSPEND_TO_RAM:
      return 1;
    default:
      return 0;
  }
}

static uint32_t prv_cpu_id(void) {
#if CONFIG_MP_MAX_NUM_CPUS == 1
  return 0;
#else
  return arch_proc_id();
#endif
}

//! Lowest substate ID that has a metric column for the given state.
static uint8_t prv_first_tracked_substate_id(enum pm_state state) {
  return (state == PM_STATE_SUSPEND_TO_IDLE) ? MFLT_PM_METRICS_S2IDLE_FIRST_TRACKED_SUBSTATE_ID : 0;
}

//! Maps an SoC substate ID to its metric column, clamped into
//! [0, num_slots). Returns false if this substate is deliberately not
//! tracked, in which case *index_out is left untouched.
//!
//! Caller must ensure num_slots > 0 (i.e. that this state is actually
//! tracked - see prv_num_tracked_substates()); this does not handle
//! num_slots == 0.
static bool prv_substate_index(enum pm_state state, uint8_t substate_id, uint8_t num_slots,
                               uint8_t *index_out) {
  const uint8_t first_tracked_id = prv_first_tracked_substate_id(state);
  if (substate_id < first_tracked_id) {
    return false;
  }

  const uint8_t column = (uint8_t)(substate_id - first_tracked_id);
  *index_out = (column >= num_slots) ? (uint8_t)(num_slots - 1) : column;
  return true;
}

static void prv_pm_substate_entry(enum pm_state state, uint8_t substate_id) {
  ARG_UNUSED(state);
  ARG_UNUSED(substate_id);
  s_mflt_pm_state_entry_ticks[prv_cpu_id()] = k_uptime_ticks();
}

static void prv_pm_substate_exit(enum pm_state state, uint8_t substate_id) {
  if ((state <= PM_STATE_ACTIVE) || (state >= PM_STATE_COUNT)) {
    return;
  }

  const uint8_t num_slots = prv_num_tracked_substates(state);
  if (num_slots == 0) {
    return;
  }

  uint8_t substate_index;
  if (!prv_substate_index(state, substate_id, num_slots, &substate_index)) {
    return;
  }

  const int64_t elapsed_ticks = k_uptime_ticks() - s_mflt_pm_state_entry_ticks[prv_cpu_id()];
  if (elapsed_ticks <= 0) {
    return;
  }

  const uint32_t elapsed_ms = (uint32_t)k_ticks_to_ms_floor64(elapsed_ticks);
  atomic_add(&s_mflt_pm_state_ticks[state][substate_index], (atomic_val_t)elapsed_ms);
}

static struct pm_notifier s_mflt_pm_notifier = {
  .substate_entry = prv_pm_substate_entry,
  .substate_exit = prv_pm_substate_exit,
  .report_substate = true,
};

void memfault_zephyr_power_metrics_collect(void) {
  for (enum pm_state state = PM_STATE_RUNTIME_IDLE; state < PM_STATE_COUNT; state++) {
    const uint8_t num_slots = prv_num_tracked_substates(state);
    for (uint8_t column = 0; column < num_slots; column++) {
      // atomic_set() both reads the accumulated value and resets it to 0 in
      // a single atomic op, so no time is lost to (or double-counted from) a
      // concurrent atomic_add() in a PM notifier callback.
      const atomic_val_t accumulated_ms = atomic_set(&s_mflt_pm_state_ticks[state][column], 0);
      memfault_metrics_heartbeat_set_unsigned(g_mflt_pm_state_metric_keys[state][column],
                                              (uint32_t)accumulated_ms);
    }
  }
}

static int prv_init_power_metrics(void) {
  pm_notifier_register(&s_mflt_pm_notifier);
  return 0;
}
SYS_INIT(prv_init_power_metrics, APPLICATION, CONFIG_MEMFAULT_INIT_PRIORITY);
