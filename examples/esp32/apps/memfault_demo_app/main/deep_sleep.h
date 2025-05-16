//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Deep sleep handling

#pragma once

#include <stdint.h>

#include "esp_err.h"

//! Call this to start deep sleep
//! @param sleep_seconds The number of seconds to sleep for
esp_err_t deep_sleep_start(uint32_t sleep_seconds);

//! Called on boot to process wakeup from deep sleep. Should be called on every
//! boot.
void deep_sleep_wakeup(void);
