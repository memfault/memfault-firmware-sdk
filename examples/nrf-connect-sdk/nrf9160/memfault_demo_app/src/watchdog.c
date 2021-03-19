//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example configuration of Zephyr hardware watchdog with Memfault software watchdog
//! port such that a coredump is captured ahead of the hardware watchdog firing

#include <zephyr.h>
#include <device.h>
#include <version.h>

#include <drivers/watchdog.h>

#include "memfault/core/debug_log.h"
#include "memfault/ports/watchdog.h"

//! Note: The timeout must be large enough to give us enough time to capture a coredump
//! before the system resets
#define MEMFAULT_WATCHDOG_HW_TIMEOUT_SECS (MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS + 10)

#define WDT_MAX_WINDOW  (MEMFAULT_WATCHDOG_HW_TIMEOUT_SECS * 1000)

#define WATCHDOG_TASK_STACK_SIZE 512
K_THREAD_STACK_DEFINE(s_wdt_task_stack_area, WATCHDOG_TASK_STACK_SIZE);
struct k_thread my_thread_data;

#if KERNEL_VERSION_MAJOR == 2 && KERNEL_VERSION_MINOR < 2
static struct device *s_wdt = NULL;
#else
static const struct device *s_wdt = NULL;
#endif

#if KERNEL_VERSION_MAJOR == 2 && KERNEL_VERSION_MINOR < 3
#define WDT_DEV_NAME DT_WDT_0_NAME
#else
#define WDT_NODE DT_INST(0, nordic_nrf_watchdog)
#define WDT_DEV_NAME DT_LABEL(WDT_NODE)
#endif

static int s_wdt_channel_id = -1;

void memfault_demo_app_watchdog_feed(void) {
  if ((s_wdt == NULL) || (s_wdt_channel_id < 0)) {
    return;
  }

  wdt_feed(s_wdt, s_wdt_channel_id);
  memfault_software_watchdog_feed();
}

//! A basic watchdog implementation
//!
//! Once Zephyr has a Software & Task watchdog in place, the example will be updated to make use of that
//! For more info about watchdog setup in general, see https://mflt.io/root-cause-watchdogs
static void prv_wdt_task(void *arg1, void *arg2, void *arg3) {
  while (1) {
    k_sleep(K_SECONDS(1));

    memfault_demo_app_watchdog_feed();
  }
}

void memfault_demo_app_watchdog_boot(void) {
  MEMFAULT_LOG_DEBUG("Initializing WDT Subsystem");

  s_wdt = device_get_binding(WDT_DEV_NAME);
  if (s_wdt == NULL) {
    printk("No WDT device available\n");
    return;
  }

  struct wdt_timeout_cfg wdt_config = {
    /* Reset SoC when watchdog timer expires. */
    .flags = WDT_FLAG_RESET_SOC,

    /* Expire watchdog after max window */
    .window.min = 0U,
    .window.max = WDT_MAX_WINDOW,
  };

  s_wdt_channel_id = wdt_install_timeout(s_wdt, &wdt_config);
  if (s_wdt_channel_id < 0) {
    MEMFAULT_LOG_ERROR("Watchdog install error, rv=%d", s_wdt_channel_id);
    return;
  }

  const uint8_t options = WDT_OPT_PAUSE_HALTED_BY_DBG;
  int err = wdt_setup(s_wdt, options);
  if (err < 0) {
    MEMFAULT_LOG_ERROR("Watchdog setup error, rv=%d", err);
    return;
  }

  // start a low priority task that feeds the watchdog.
  // We make it the lowest priority so a hot loop in any task above will
  // cause the watchdog to not be fed
  memfault_software_watchdog_enable();
  k_thread_create(&my_thread_data, s_wdt_task_stack_area,
                  K_THREAD_STACK_SIZEOF(s_wdt_task_stack_area),
                  prv_wdt_task,
                  NULL, NULL, NULL,
                  K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
}
