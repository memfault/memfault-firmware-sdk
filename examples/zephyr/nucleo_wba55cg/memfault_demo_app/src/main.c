//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "memfault/components.h"
#include "memfault/ports/zephyr/http.h"

LOG_MODULE_REGISTER(mflt_app, LOG_LEVEL_DBG);

// Blink code taken from the zephyr/samples/basic/blinky example.

static void blink_forever(void) {
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
}

// put the blink_forever in its own thread.
K_THREAD_DEFINE(blink_forever_thread, 1024, blink_forever, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0,
                0);

int main(void) {
  printk(MEMFAULT_BANNER_COLORIZED "\n");

  LOG_INF("Memfault Demo App! Board %s\n", CONFIG_BOARD);
  memfault_device_info_dump();

  return 0;
}
