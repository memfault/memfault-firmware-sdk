//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! App settings helper functions.

#pragma once

#include <stddef.h>

#include "memfault/esp_port/version.h"

enum settings_key {
  kSettingsWifiSsid,
  kSettingsWifiPassword,
  kSettingsProjectKey,
  kSettingsLedBrightness,
  kSettingsLedBlinkIntervalMs,
};

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #if __GNUC__ >= 11
__attribute__((access(write_only, 2)))
  #endif
  int settings_get(enum settings_key key, void *value, size_t *len);
  #if __GNUC__ >= 11
__attribute__((access(read_only, 2, 3)))
  #endif
  int settings_set(enum settings_key key, const void *value, size_t len);
void settings_register_shell_commands(void);
#else   // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
// stub definitions that always fail
static inline int settings_get(enum settings_key key, void *value, size_t *len) {
  return -1;
}
static inline int settings_set(enum settings_key key, const void *value, size_t len) {
  return -1;
}
static inline void settings_register_shell_commands(void) {}
#endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
