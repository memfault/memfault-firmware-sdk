//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Module for controlling the RGB LED. Implementation varies depending on which
//! target board; the ESP32-WROVER has a 3-gpio LED, while the ESP32-S3-DevKitC
//! has an addressable LED.

#include "led.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "settings.h"

#if CONFIG_BLINK_LED_RMT
  #include "led_strip.h"
#endif

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "memfault/components.h"

// System state LED color:
// Red:   System is running, has not checked in to memfault (wifi might be bad)
// Green: System is running, has checked in to memfault
// Blue:  System is performing an OTA update
static int s_led_color = kLedColor_Red;

static int32_t s_led_brightness = 5;

void led_set_color(enum LED_COLORS color) {
  s_led_color = color;
}

#if CONFIG_BLINK_LED_RMT
struct rgb_led_s {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static led_strip_handle_t led_strip;

static void led_config(void) {
  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
    .strip_gpio_num = CONFIG_BLINK_GPIO,
    .max_leds = 1,  // at least one LED on board
  };
  led_strip_rmt_config_t rmt_config = {
    .resolution_hz = 10 * 1000 * 1000,  // 10MHz
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  /* Set all LED off to clear all pixels */
  led_strip_clear(led_strip);
}

static void prv_set_pixel(struct rgb_led_s rgb, bool set) {
  /* If the addressable LED is enabled */
  if (set) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_set_pixel(led_strip, 0, rgb.r, rgb.g, rgb.b);
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
  } else {
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
  }
}

  #else   // ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)

static led_strip_t *pStrip_a;

static void prv_set_pixel(struct rgb_led_s rgb, bool set) {
  if (set) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    pStrip_a->set_pixel(pStrip_a, 0, rgb.r, rgb.g, rgb.b);
    /* Refresh the strip to send data */
    pStrip_a->refresh(pStrip_a, 100);
  } else {
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);
  }
}

static void led_config(void) {
  /* LED strip initialization with the GPIO and pixels number*/
  pStrip_a = led_strip_init(CONFIG_BLINK_LED_RMT_CHANNEL, CONFIG_BLINK_GPIO, 1);
  /* Set all LED off to clear all pixels */
  pStrip_a->clear(pStrip_a, 50);
}
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

static void prv_heartbeat_led_callback(MEMFAULT_UNUSED TimerHandle_t handle) {
  static bool s_led_state = true;

  /* If the addressable LED is enabled */
  struct rgb_led_s rgb_led;

  switch (s_led_color) {
    default:
    case kLedColor_Red:
      rgb_led.r = s_led_brightness;
      rgb_led.g = 0;
      rgb_led.b = 0;
      break;
    case kLedColor_Green:
      rgb_led.r = 0;
      rgb_led.g = s_led_brightness;
      rgb_led.b = 0;
      break;
    case kLedColor_Blue:
      rgb_led.r = 0;
      rgb_led.g = 0;
      rgb_led.b = s_led_brightness;
      break;
  }

  prv_set_pixel(rgb_led, s_led_state);
  s_led_state = !s_led_state;
}

#else  // CONFIG_BLINK_LED_RMT
static void prv_heartbeat_led_callback(MEMFAULT_UNUSED TimerHandle_t handle) {
  static bool s_led_state = false;
  s_led_state = !s_led_state;

  const gpio_num_t leds[] = {kLedColor_Red, kLedColor_Green, kLedColor_Blue};
  for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); i++) {
    if (leds[i] == s_led_color) {
      gpio_set_level(s_led_color, s_led_state);
    } else {
      gpio_set_level(leds[i], 0);
    }
  }
}

static void led_config(void) {
  const gpio_num_t leds[] = {kLedColor_Red, kLedColor_Green, kLedColor_Blue};

  for (size_t i = 0; i < sizeof(leds) / sizeof(leds[0]); i++) {
    gpio_reset_pin(leds[i]);
    gpio_set_direction(leds[i], GPIO_MODE_OUTPUT);
    gpio_set_level(leds[i], 0);
  }
}

#endif  // CONFIG_BLINK_LED_RMT

void led_init(void) {
  led_config();

  size_t len = sizeof(s_led_brightness);
  (void)settings_get(kSettingsLedBrightness, &s_led_brightness, &len);
  ESP_LOGI(__func__, "LED brightness: %" PRIi32, s_led_brightness);

  // create a timer that blinks the LED, indicating the app is alive
  const char *const pcTimerName = "HeartbeatLED";
  int32_t led_interval_ms = 500;
  len = sizeof(led_interval_ms);
  (void)settings_get(kSettingsLedBlinkIntervalMs, &led_interval_ms, &len);
  ESP_LOGI(__func__, "LED blink interval: %" PRIi32, led_interval_ms);
  const TickType_t xTimerPeriodInTicks = pdMS_TO_TICKS(led_interval_ms);

  TimerHandle_t timer;

#if MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticTimer_t s_heartbeat_led_timer_context;
  timer = xTimerCreateStatic(pcTimerName, xTimerPeriodInTicks, pdTRUE, NULL,
                             prv_heartbeat_led_callback, &s_heartbeat_led_timer_context);
#else
  timer = xTimerCreate(pcTimerName, xTimerPeriodInTicks, pdTRUE, NULL, prv_heartbeat_led_callback);
#endif

  MEMFAULT_ASSERT(timer != 0);

  xTimerStart(timer, 0);
}
