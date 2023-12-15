#pragma once

//! @file
//!
//! @brief
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_FAMILY_ESP32)
  #define ZEPHYR_DATA_REGION_START _data_start
  #define ZEPHYR_DATA_REGION_END _data_end
#endif

#ifdef __cplusplus
}
#endif
