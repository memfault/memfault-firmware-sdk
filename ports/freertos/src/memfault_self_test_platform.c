//! @file

// Espressif's esp-idf project uses a different include directory by default.
#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #if !defined(CONFIG_IDF_TARGET_ESP8266)
    #define MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #endif
#endif

#if defined(MEMFAULT_USE_ESP32_FREERTOS_INCLUDE)
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#else  // MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
  #include "FreeRTOS.h"
  #include "task.h"
#endif  // MEMFAULT_USE_ESP32_FREERTOS_INCLUDE

void memfault_self_test_platform_delay(uint32_t delay_ms) {
  vTaskDelay(delay_ms / portTICK_PERIOD_MS);
}

#if defined(ESP_PLATFORM)
void memfault_include_self_test_impl(void) { }
#endif
