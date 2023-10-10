//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Reference implementation of Memfault device info API platform dependencies for the ESP32

#include <stdio.h>

#include "memfault/components.h"
#include "memfault/esp_port/version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
  #include "esp_mac.h"
#endif

#include <string.h>

#include "esp_system.h"
#include "memfault/esp_port/device_info.h"

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
    } else {  // we are out of space, return what we got, it's NULL terminated
      return;
    }
  }
}

void memfault_esp_port_get_device_info(struct MemfaultDeviceInfo *info) {
  // Initialize the device information data structure if the s_device_serial is
  // not set. Note that the first call to this function should be in a
  // non-interrupt context, to safely load the mac address.
  char *device_serial = s_device_serial;
  if (s_device_serial[0] == '\0') {
    // if in isr, don't attempt to read mac address. just set the device serial
    // to "unknown" and continue
    if (memfault_arch_is_inside_isr()) {
      device_serial = "unknown";
    } else {
      prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
    }
  }
  *info = (struct MemfaultDeviceInfo){
    .device_serial = device_serial,
    .hardware_version = CONFIG_MEMFAULT_DEVICE_INFO_HARDWARE_VERSION,
    .software_version = CONFIG_MEMFAULT_DEVICE_INFO_SOFTWARE_VERSION,
    .software_type = CONFIG_MEMFAULT_DEVICE_INFO_SOFTWARE_TYPE,
  };
}

#if defined(CONFIG_MEMFAULT_DEFAULT_GET_DEVICE_INFO)
void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  memfault_esp_port_get_device_info(info);
}
#endif
