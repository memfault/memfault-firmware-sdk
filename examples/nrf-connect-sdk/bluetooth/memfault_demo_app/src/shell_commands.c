//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Shell commands

#include <stdlib.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
// #include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/poweroff.h>

LOG_MODULE_REGISTER(appshell, LOG_LEVEL_INF);

static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

// Define a simple shell command
static int cmd_poweroff(const struct shell *shell, size_t argc, char **argv) {
  LOG_INF("Powering off device");
  /* configure sw0 as input, interrupt as level active to allow wake-up */
  int rc = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
  if (rc < 0) {
    LOG_ERR("Could not configure sw0 GPIO (%d)\n", rc);
    return 0;
  }

  rc = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_LEVEL_ACTIVE);
  if (rc < 0) {
    LOG_ERR("Could not configure sw0 GPIO interrupt (%d)\n", rc);
    return 0;
  }

  LOG_INF("Entering system off; press sw0 to restart\n");

  // rc = pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
  // if (rc < 0) {
  // 	printf("Could not suspend console (%d)\n", rc);
  // 	return 0;
  // }

  // if (IS_ENABLED(CONFIG_APP_RETENTION)) {
  // 	/* Update the retained state */
  // 	retained.off_count += 1;
  // 	retained_update();
  // }

  sys_poweroff();
  return 0;
}

SHELL_CMD_REGISTER(poweroff, NULL, "Power off the device", cmd_poweroff);

#if defined(CONFIG_PM)
//! Forces the CPU into PM_STATE_SUSPEND_TO_IDLE for a caller-selected
//! duration, to exercise the "power_suspend_to_idle_sN_time_ms" Memfault
//! metrics (N = substate_id) on demand. Substate 0 has no metric - it isn't
//! implemented in the PM code, so it's never entered.
//!
//! pm_state_force() only applies to the *next* idle entry, so this only
//! guarantees the forced state is used if nothing else wakes the CPU (e.g.
//! other timers, log flushing) before the k_msleep() below completes. For a
//! clean single measurement, keep other activity on the board quiet while
//! testing (e.g. disable periodic Memfault uploads).
static int cmd_pm_suspend_to_idle(const struct shell *shell, size_t argc, char **argv) {
  if (argc < 2) {
    shell_error(shell, "Usage: %s <duration_ms> [substate_id]", argv[0]);
    return -EINVAL;
  }

  const uint32_t duration_ms = (uint32_t)strtoul(argv[1], NULL, 0);
  // substate 2 ("idle_cache_disabled") is the deepest SUSPEND_TO_IDLE
  // substate implemented today on nRF54H/nRF92 (see soc_power.c); substate 1
  // ("idle_cache_retained") is also available. See the board's devicetree
  // "power-states" node for the substates this target actually supports.
  const uint8_t substate_id = (argc > 2) ? (uint8_t)strtoul(argv[2], NULL, 0) : 2;

  const struct pm_state_info info = {
    .state = PM_STATE_SUSPEND_TO_IDLE,
    .substate_id = substate_id,
  };

  if (!pm_state_force(0, &info)) {
    shell_error(shell,
                "Failed to force PM_STATE_SUSPEND_TO_IDLE substate %u - "
                "check the board's devicetree \"power-states\" node",
                substate_id);
    return -EIO;
  }

  shell_print(shell, "Forcing PM_STATE_SUSPEND_TO_IDLE substate %u for %u ms", substate_id,
              duration_ms);
  k_msleep(duration_ms);
  // Substate IDs >= 3 fold into the "s2" metric bucket - see
  // MFLT_PM_METRICS_MAX_SUBSTATES in memfault_platform_power_metrics.c.
  if (substate_id == 0) {
    shell_print(shell, "Done - substate 0 has no metric, no time was recorded");
  } else {
    shell_print(shell, "Done - check the \"power_suspend_to_idle_s%u_time_ms\" Memfault metric",
                MIN(substate_id, 2));
  }

  return 0;
}

SHELL_CMD_ARG_REGISTER(pm_suspend_to_idle, NULL,
                       "Force PM_STATE_SUSPEND_TO_IDLE for a duration in ms: "
                       "pm_suspend_to_idle <ms> [substate_id]",
                       cmd_pm_suspend_to_idle, 2, 1);
#endif  // defined(CONFIG_PM)
