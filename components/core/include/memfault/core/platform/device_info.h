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
  const char *device_serial;
  const char *fw_version;
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
