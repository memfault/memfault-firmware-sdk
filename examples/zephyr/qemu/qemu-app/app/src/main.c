//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example app main

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <zephyr.h>

#include "memfault/components.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Blink code taken from the zephyr/samples/basic/blinky example.
static void blink_forever(void) {
#if CONFIG_QEMU_TARGET
  k_sleep(K_FOREVER);
#else
  /* 1000 msec = 1 sec */
  #define SLEEP_TIME_MS 1000

  /* The devicetree node identifier for the "led0" alias. */
  #define LED0_NODE DT_ALIAS(led0)

  static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return;
  }

  while (1) {
    gpio_pin_toggle_dt(&led);
    k_msleep(SLEEP_TIME_MS);
  }
#endif  // CONFIG_QEMU_TARGET
}

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-app",
    .software_version = "1.0.0+" ZEPHYR_MEMFAULT_EXAMPLE_GIT_SHA1,
    .hardware_version = CONFIG_BOARD,
  };
}

#if CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_THREAD_TOGGLE
K_THREAD_STACK_DEFINE(test_thread_stack_area, 1024);
static struct k_thread test_thread;

static void prv_test_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  k_sleep(K_FOREVER);
}

static void prv_test_thread_work_handler(struct k_work *work) {
  ARG_UNUSED(work);

  static bool started = false;
  if (started) {
    LOG_INF("ending test_thread ❌");
    k_thread_abort(&test_thread);
    started = false;
  } else {
    LOG_INF("starting test_thread ✅");
    k_thread_create(&test_thread, test_thread_stack_area,
                    K_THREAD_STACK_SIZEOF(test_thread_stack_area), prv_test_thread_function, NULL,
                    NULL, NULL, 7, 0, K_FOREVER);
    k_thread_name_set(&test_thread, "test_thread");
    k_thread_start(&test_thread);
    started = true;
  }
}
K_WORK_DEFINE(s_test_thread_work, prv_test_thread_work_handler);

//! Timer handlers run from an ISR so we dispatch the heartbeat job to the
//! worker task
static void prv_test_thread_timer_expiry_handler(struct k_timer *dummy) {
  k_work_submit(&s_test_thread_work);
}
K_TIMER_DEFINE(s_test_thread_timer, prv_test_thread_timer_expiry_handler, NULL);

static void prv_init_test_thread_timer(void) {
  k_timer_start(&s_test_thread_timer, K_SECONDS(10), K_SECONDS(10));

  // fire one time right away
  k_work_submit(&s_test_thread_work);
}
#else
static void prv_init_test_thread_timer(void) {}
#endif  // CONFIG_ZEPHYR_MEMFAULT_EXAMPLE_THREAD_TOGGLE

void main(void) {
  LOG_INF("👋 Memfault Demo App! Board %s\n", CONFIG_BOARD);
  memfault_device_info_dump();

  prv_init_test_thread_timer();

  blink_forever();
}
