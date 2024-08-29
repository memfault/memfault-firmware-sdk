//! @file
//!
//! Shell commands

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
// #include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
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
