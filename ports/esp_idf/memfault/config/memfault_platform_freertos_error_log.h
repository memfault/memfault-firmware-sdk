#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Implement a special MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG() macro used
//! when logging from the FreeRTOS task creation function.

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"

// Use the ESP-IDF logging 'ESP_DRAM_LOGx' log variant instead of the default
// 'MEMFAULT_LOG_ERROR' macro. This should be safe to call in interrupt context
// or when the logging subsystem has not been initialized (early init).
#define MEMFAULT_FREERTOS_REGISTRY_FULL_ERROR_LOG(...) ESP_DRAM_LOGE("mflt", __VA_ARGS__)

#ifdef __cplusplus
}
#endif
