#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Memfault splash screen banner.

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
#define MEMFAULT_BANNER_(MEMFAULT_COLOR_START, MEMFAULT_COLOR_END) \
  "▙▗▌       ▗▀▖      ▜▐   " MEMFAULT_COLOR_START "  ▄▄▀▀▄▄ " MEMFAULT_COLOR_END "\n" \
  "▌▘▌▞▀▖▛▚▀▖▐  ▝▀▖▌ ▌▐▜▀  " MEMFAULT_COLOR_START " █▄    ▄█" MEMFAULT_COLOR_END "\n" \
  "▌ ▌▛▀ ▌▐ ▌▜▀ ▞▀▌▌ ▌▐▐ ▖ " MEMFAULT_COLOR_START " ▄▀▀▄▄▀▀▄" MEMFAULT_COLOR_END "\n" \
  "▘ ▘▝▀▘▘▝ ▘▐  ▝▀▘▝▀▘ ▘▀  " MEMFAULT_COLOR_START "  ▀▀▄▄▀▀ " MEMFAULT_COLOR_END "\n"
// clang-format on

//! Memfault banner with colorized logo
#define MEMFAULT_BANNER_COLORIZED MEMFAULT_BANNER_("\e[36m", "\e[0m")

//! Memfault banner with monochrome logo
#define MEMFAULT_BANNER_MONOCHROME MEMFAULT_BANNER_("", "")

#ifdef __cplusplus
}
#endif
