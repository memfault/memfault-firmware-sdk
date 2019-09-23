#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! @details
//!
//! @brief
//! Platform APIs used to get information about the device and its components.

#include <stdbool.h>
#include <stddef.h>

#define MEMFAULT_MAX_SERIAL_NUMBER_LEN 17
#define MEMFAULT_MAX_HW_REVISION_LEN 17
#define MEMFAULT_MAX_COMPONENT_VERSION_LEN 17

#ifdef __cplusplus
extern "C" {
#endif

struct MemfaultDeviceInfo {
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
};

//! Invoked by memfault library to query the device information
//!
//! It's expected the strings returned will be valid for the lifetime of the application
void memfault_platform_get_device_info(struct MemfaultDeviceInfo *info);

//! Invoked by query to copy device serial into provided buffer.
void memfault_platform_get_unique_device_id(char *buf, size_t buf_len);

//! TODO: To use this service for other devices, we need to think about how the
//! 'ComponentVersion' enum values should be structured
typedef enum {
  kMfltComponentVersionType_MainFirmware = 0x00,
  kMfltComponentVersionType_BluetoothRadio = 0x01,
  kMfltComponentVersionType_Bootloader = 0x02,
  kMfltComponentVersionType_HardwareRevision = 0x03,
} eMfltComponentVersionType;

//! Populate buffer with a NUL terminate string encoding the current version of the software
//! component requested
//!
//! @return true if the version was populated, false otherwise
bool memfault_platform_get_component_version(eMfltComponentVersionType type, char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif
