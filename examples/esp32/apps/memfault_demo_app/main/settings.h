//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! App settings helper functions.

#pragma once

#include <stddef.h>

enum settings_key {
  kSettingsWifiSsid,
  kSettingsWifiPassword,
  kSettingsProjectKey,
  kSettingsLedBrightness,
  kSettingsLedBlinkIntervalMs,
};

__attribute__((access(write_only, 2))) int settings_get(enum settings_key key, void *value,
                                                        size_t *len);
__attribute__((access(read_only, 2, 3))) int settings_set(enum settings_key key, const void *value,
                                                          size_t len);
void settings_register_shell_commands(void);
