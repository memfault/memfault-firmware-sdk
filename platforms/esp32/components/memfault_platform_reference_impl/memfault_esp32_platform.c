#include <stdio.h>

#include "memfault/core/platform/device_info.h"

#include "esp_system.h"

#include <string.h>

#ifndef MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION
#  define MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION "1.0.0"
#endif

#ifndef MEMFAULT_ESP32_HW_REVISION
#  define MEMFAULT_ESP32_HW_REVISION "esp32-proto"
#endif

void memfault_platform_get_unique_device_id(char *buf, size_t buf_len) {
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

void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info) {
  static char s_fw_version[32];
  memfault_platform_get_component_version(kMfltComponentVersionType_MainFirmware, s_fw_version,
                                 sizeof(s_fw_version));

  static char s_device_serial[MEMFAULT_MAX_SERIAL_NUMBER_LEN];
  memfault_platform_get_unique_device_id(s_device_serial, sizeof(s_device_serial));


  static char s_hw_rev[MEMFAULT_MAX_HW_REVISION_LEN];
  memfault_platform_get_component_version(kMfltComponentVersionType_HardwareRevision, s_hw_rev,
                                 sizeof(s_hw_rev));

  *info = (struct MemfaultDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = s_hw_rev,
    .fw_version = s_fw_version,
  };
}

bool memfault_platform_get_component_version(eMfltComponentVersionType type, char *buf, size_t buf_len) {
  switch (type) {
    case kMfltComponentVersionType_MainFirmware:
      strncpy(buf, MEMFAULT_ESP32_MAIN_FIRMWARE_VERSION, buf_len);
      break;
    case kMfltComponentVersionType_HardwareRevision:
      strncpy(buf, MEMFAULT_ESP32_HW_REVISION, buf_len);
      break;
    default:
      return false;
  }

  return true;
}
