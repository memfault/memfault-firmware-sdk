//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
#include <stdbool.h>

// Espressif's esp-idf project uses a different include directory by default.
#if defined(ESP_PLATFORM)
  #include "sdkconfig.h"
  #define MEMFAULT_USE_ESP32_FREERTOS_INCLUDE
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

#if defined(CONFIG_MEMFAULT_CLI_SELF_TEST_COREDUMP_STORAGE)
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;

bool memfault_self_test_platform_disable_irqs(void) {
  // First prevent any ISRs local to this core from running by entering critical section
  portENTER_CRITICAL(&lock);

  // Next reset and stall the other core if we're not in single core mode
  // Reset required, see
  // esp-idf/components/esp_system/port/soc/<soc>/system_internal.c:esp_restart_noos for details
  #if !defined(CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)
  int core_id = esp_cpu_get_core_id();
  for (uint32_t i = 0; i < SOC_CPU_CORES_NUM; i++) {
    if (i != core_id) {
      esp_cpu_reset(i);
      esp_cpu_stall(i);
    }
  }
  #endif  // !defined(CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)
  return true;
}

bool memfault_self_test_platform_enable_irqs(void) {
  // First unstall the other core before exiting the critical section
  #if !defined(CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)
  int core_id = esp_cpu_get_core_id();
  for (uint32_t i = 0; i < SOC_CPU_CORES_NUM; i++) {
    if (i != core_id) {
      esp_cpu_unstall(i);
    }
  }
  #endif  // !defined(CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE)

  // Exit the critical section entered by memfault_self_test_platform_disable_irqs
  portEXIT_CRITICAL(&lock);
  return true;
}
#endif

// Workaround to allow strong definition of memfault_self_test_platform functions to be linked when
// building with esp-idf
void memfault_esp_idf_include_self_test_impl(void) { }
