//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! App settings helper functions.

#pragma once

#include <stddef.h>

#include "esp_idf_version.h"

enum settings_key {
  kSettingsWifiSsid,
  kSettingsWifiPassword,
  kSettingsProjectKey,
  kSettingsLedBrightness,
  kSettingsLedBlinkIntervalMs,
  kSettingsChunksUrl,
  kSettingsDeviceUrl,
};

#if __GNUC__ >= 11
__attribute__((access(write_only, 2)))
#endif
int
settings_get(enum settings_key key, void *value, size_t *len);
#if __GNUC__ >= 11
__attribute__((access(read_only, 2, 3)))
#endif
int
settings_set(enum settings_key key, const void *value, size_t len);
void settings_register_shell_commands(void);
