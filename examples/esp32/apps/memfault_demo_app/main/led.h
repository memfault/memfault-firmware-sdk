//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! LED control module header.

#pragma once

enum LED_COLORS {
  kLedColor_Red = 0,
  kLedColor_Green = 2,
  kLedColor_Blue = 4,
};

void led_set_color(enum LED_COLORS color);

void led_init(void);
