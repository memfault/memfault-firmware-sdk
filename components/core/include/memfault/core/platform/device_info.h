#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//! @details
//!
//! @brief
//! Platform APIs used to get information about the device and its components.

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemfaultDeviceInfo {
  //! The device's serial number or unique identifier.
  //! Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
  const char *device_serial;

  //! The "Software Type" is used to identify the separate pieces of software running on a given device. This can be
  //! images running on different MCUs (i.e "main-mcu-app" & "bluetooth-mcu") or different images running on the same
  //! MCU (i.e "main-mcu-bootloader" & "main-mcu-app").
  //! To learn more, check out the documentation: https://mflt.io/34PyNGQ
  const char *software_type;

  //! Version of the currently running software.
  //! We recommend using Semantic Version V2 strings.
  const char *software_version;

  //! Hardware version (sometimes also called "board revision") that the software is currently running on.
  //! Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
  const char *hardware_version;
} sMemfaultDeviceInfo;

//! Invoked by memfault library to query the device information
//!
//! It's expected the strings returned will be valid for the lifetime of the application
void memfault_platform_get_device_info(sMemfaultDeviceInfo *info);

#ifdef __cplusplus
}
#endif
