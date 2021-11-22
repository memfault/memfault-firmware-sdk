//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Reference implementation of Memfault device info API platform dependencies for the ESP32

#include <stdio.h>

#include "memfault/components.h"

#include "esp_system.h"

#include <string.h>

#ifndef MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION
#  define MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION "1.0.0"
#endif

#ifndef MEMFAULT_ESP32_SOFTWARE_TYPE
#  define MEMFAULT_ESP32_SOFTWARE_TYPE "esp32-main"
#endif

#ifndef MEMFAULT_ESP32_HW_REVISION
#  define MEMFAULT_ESP32_HW_REVISION "esp32-proto"
#endif

static char s_device_serial[32];

// NOTE: Some versions of the esp-idf use locking when reading mac info
// so this isn't safe to call from an interrupt
static void prv_get_device_serial(char *buf, size_t buf_len) {
  // Use the ESP32's MAC address as unique device id:
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  size_t curr_idx = 0;
  for (size_t i = 0; i < sizeof(mac); i++) {
    size_t space_left = buf_len - curr_idx;
    int bytes_written = snprintf(&buf[curr_idx], space_left, "%02X", (int)mac[i]);
    if (bytes_written < space_left) {
      curr_idx += bytes_written;
    } else { // we are out of space, return what we got, it's NULL terminated
      return;
    }
  }
}

void memfault_platform_device_info_boot(void) {
  prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
}

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = MEMFAULT_ESP32_HW_REVISION,
    .software_version = MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION "-dev",
    .software_type = MEMFAULT_ESP32_SOFTWARE_TYPE,
  };
}
