//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Button setup and handling

#include "button.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "button";

static void IRAM_ATTR prv_gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;

  // dereference a null point to trigger a crash
  volatile uint32_t *ptr = NULL;
  *ptr = gpio_num;
}

// The flex glitch filter is only available on 5.1. Skip it for earlier SDKs.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
  #include "driver/gpio_filter.h"
static void prv_button_glitch_filter_enable(void) {
  #if SOC_GPIO_FLEX_GLITCH_FILTER_NUM > 0
  gpio_glitch_filter_handle_t filter;
  gpio_flex_glitch_filter_config_t filter_cfg = {
    .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
    .gpio_num = CONFIG_BUTTON_GPIO,
    .window_thres_ns = 500,
    .window_width_ns = 500,
  };
  esp_err_t err = gpio_new_flex_glitch_filter(&filter_cfg, &filter);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create glitch filter: %s", esp_err_to_name(err));
    return;
  }

  err = gpio_glitch_filter_enable(filter);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to enable glitch filter: %s", esp_err_to_name(err));
    return;
  }
  #endif
}
#else   // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
static void prv_button_glitch_filter_enable(void) {
  // No-op
}
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)

void button_setup(void) {
  // configure the button as an input
  gpio_config_t io_conf = {
    .intr_type = GPIO_INTR_NEGEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = 1ULL << CONFIG_BUTTON_GPIO,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE,
  };
  esp_err_t err = gpio_config(&io_conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure button: %s", esp_err_to_name(err));
    return;
  }

  prv_button_glitch_filter_enable();

  // install gpio isr service
  err = gpio_install_isr_service(0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install gpio isr service: %s", esp_err_to_name(err));
    return;
  }

  // install isr handler for specific gpio pin
  err = gpio_isr_handler_add(CONFIG_BUTTON_GPIO, prv_gpio_isr_handler, (void *)CONFIG_BUTTON_GPIO);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add isr handler for button: %s", esp_err_to_name(err));
    return;
  }
}
