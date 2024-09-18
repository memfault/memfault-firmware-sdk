//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
#include <stdbool.h>

#include "FreeRTOS.h"
#include "memfault/core/self_test.h"
#include "task.h"

void memfault_self_test_platform_delay(uint32_t delay_ms) {
  vTaskDelay(delay_ms / portTICK_PERIOD_MS);
}

bool memfault_self_test_platform_disable_irqs(void) {
  taskENTER_CRITICAL();
  return true;
}

bool memfault_self_test_platform_enable_irqs(void) {
  taskEXIT_CRITICAL();
  return true;
}
