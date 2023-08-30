//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example configuration of Zephyr hardware watchdog with Memfault software watchdog
//! port such that a coredump is captured ahead of the hardware watchdog firing

// clang-format off
#include "memfault/ports/watchdog.h"

#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(device.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/watchdog.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include <version.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/version.h"
// clang-format on

//! Note: The timeout must be large enough to give us enough time to capture a coredump
//! before the system resets
#define MEMFAULT_WATCHDOG_HW_TIMEOUT_SECS (MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS + 10)

#define WDT_MAX_WINDOW (MEMFAULT_WATCHDOG_HW_TIMEOUT_SECS * 1000)

#define WATCHDOG_TASK_STACK_SIZE 512
K_THREAD_STACK_DEFINE(s_wdt_task_stack_area, WATCHDOG_TASK_STACK_SIZE);
struct k_thread my_thread_data;

#if MEMFAULT_ZEPHYR_VERSION_GT(3,1)
// Between Zephyr 3.1 and 3.2, "label" properties in DTS started to get phased out:
//   https://github.com/zephyrproject-rtos/zephyr/pull/48360
//
// This breaks support for accessing device tree information via device_get_binding / DT_LABEL
// properties. The new preferred approach is accessing resources via the DEVICE_DT_GET macro
static const struct device *s_wdt = DEVICE_DT_GET(DT_NODELABEL(wdt));

#elif MEMFAULT_ZEPHYR_VERSION_GT(2,1)
static const struct device *s_wdt = NULL;
#else // Zephyr Kernel <= 2.1
static struct device *s_wdt = NULL;
#endif

#if KERNEL_VERSION_MAJOR == 2 && KERNEL_VERSION_MINOR < 3
  #define WDT_DEV_NAME DT_WDT_0_NAME
#else
  //! Watchdog device tree name changed in NCS v2.0.0 :
  //! https://github.com/nrfconnect/sdk-zephyr/blob/12ee4d5f4b99acef542ce3977cb9078fcbb36d82/dts/arm/nordic/nrf9160_common.dtsi#L368
  //! Pick the one that's available in the current SDK version.
  #if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_wdt)
    #define WDT_NODE_NAME nordic_nrf_wdt
  #elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_watchdog)
    #define WDT_NODE_NAME nordic_nrf_watchdog
  #else
    #error "No compatible watchdog instance for this configuration!"
  #endif

  #define WDT_NODE DT_INST(0, WDT_NODE_NAME)
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
//! Once Zephyr has a Software & Task watchdog in place, the example will be updated to make use of
//! that For more info about watchdog setup in general, see https://mflt.io/root-cause-watchdogs
static void prv_wdt_task(void *arg1, void *arg2, void *arg3) {
  while (1) {
    k_sleep(K_SECONDS(1));

    memfault_demo_app_watchdog_feed();
  }
}

void memfault_demo_app_watchdog_boot(void) {
  MEMFAULT_LOG_DEBUG("Initializing WDT Subsystem");

#if MEMFAULT_ZEPHYR_VERSION_GT(3,1)
  if (!device_is_ready(s_wdt)) {
    printk("Watchdog device not ready");
    return;
  }
#else // Zephyr <= 3.1
  s_wdt = device_get_binding(WDT_DEV_NAME);
  if (s_wdt == NULL) {
    printk("No WDT device available\n");
    return;
  }
#endif

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
                  K_THREAD_STACK_SIZEOF(s_wdt_task_stack_area), prv_wdt_task, NULL, NULL, NULL,
                  K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
}
