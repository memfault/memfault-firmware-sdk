#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Thin wrapper around rom/uart which was refactored between the v3.x and v4.x esp-idf releases

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/uart.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/uart.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_ESP32_CONSOLE_UART_NUM CONFIG_ESP_CONSOLE_UART_NUM

#ifdef __cplusplus
}
#endif
