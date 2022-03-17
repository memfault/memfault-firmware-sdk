#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! API for the Bluetooth Low Energy Memfault Diagnostic GATT Service (MDS)
//!
//! This service can be used for forwarding diagnostic data collected by firmware through a BLE
//! gateway. The service is designed such that the gateway can be agnostic to the data being
//! forwarded and the URI it should be forwarded to.
//!
//! Memfault Diagnostic Service: (UUID 54220000-f6a5-4007-a371-722f4ebd8436)
//!
//!  MDS Supported Features Characteristic: (UUID 54220001-f6a5-4007-a371-722f4ebd8436)
//!   read - (length: variable*)
//!   Octet 0:
//!      bit 0-7: rsvd for future use
//!    ...
//!
//!  MDS Device Identifier Characteristic: (UUID 54220002-f6a5-4007-a371-722f4ebd8436)
//!   read - (length: variable*) returns the device identifier populated in the
//!     memfault_platform_get_device_info() dependency function
//!
//!  MDS Data URI Characteristic:          (UUID 54220003-f6a5-4007-a371-722f4ebd8436)
//!   read - (length: variable*) returns the URI diagnostic data should be forwarded to
//!
//!  MDS Authorization Characteristic:     (UUID 54220004-f6a5-4007-a371-722f4ebd8436)
//!   read - (length: variable*) returns the configured authorization to use such as
//!     "Memfault-Project-Key:YOUR_PROJECT_KEY"
//!
//!  MDS Data Export Characteristic:       (UUID 54220005-f6a5-4007-a371-722f4ebd8436)
//!   notify - must be subscribed to in order for any collected SDK data to be streamed
//!   write - (length: 1 byte) controls data export operation mode
//!    0x00 - Streaming Disabled
//!    0x01 - Streaming of any collected SDK data enabled
//!    Other: reserved for future use
//!
//!    When subscribed to and when streaming mode has been enabled via a GATT Write with response,
//!    packets will be sized to fit within negotiated MTU.
//!    The format of a packet streamed is as follows:
//!     byte 0:
//!      bits 0-4: Sequence Number A sequentially increasing counter that can be used to detect
//!       errors in the BLE stack. SN is reset to 0 anytime a disconnect takes place.
//!       Errors client can check for:
//!          - Packets are getting repeated when Last Sequence Number == Current Sequence Number
//!          - Packets are getting dropped  when abs(Last Sequence Number - Current Sequence Number) != 1
//!      bit 5-7: Reserved for future use
//!
//!     byte 1-N: opaque payload listener is responsible for forwarding to the URI specified in the Data URI
//!        Characteristic using the authorization scheme specified in the Authorization Characteristic.
//!
//!   Client Characteristic Configuration Descriptor: (UUID 00002902-0000-1000-8000-00805f9b34fb)
//!
//! * If length exceeds negotiated MTU size, it is assumed that client has implemented support for
//!   long attribute reads. (iOS & Android both support this). If long attribute reads are not
//!   supported for some reason, an MTU needs to be negotiated which exceeds the length of the
//!   Project Key and Device Identifier.
//!
//! Note 1: If additional characteristics are ever added to the service, they should be appended
//! to the table to minimize impact for clients who have not (correctly) implemented the "Service
//! Changed" characteristic in the Generic Attribute Profile Service.
//!
//! Note 2: For production applications and enhanced security and privacy, it is recommended
//! the memfault_mds_access_enabled() be implemented. See function comments below for more details.
//!
//! Note 3: This GATT service is currently under development and subject to updates and
//! enhancements in the future with no current guarantees for backward compatibility. Client
//! implementations should read the MDS Supported Features Characteristic and only attempt to use
//! the service as it is currently defined if the value is 0x00.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "memfault/components.h"

//! Initializes the MDS Gatt Service
//!
//! @return A reference to the service which was initialized
void *mds_boot(void);

//! Controls whether or not client can access diagnostic data
//!
//! This function exists so an end user can restrict access to the data available from the
//! diagnostic service. The default behavior is to always return true (data available on all
//! connections).
//!
//! @note We recommend implement this dependency function for production applications
//! and only allowing access to the diagnostic data when the connected peer has been
//! authenticated and the connection is encrypted.
//!
//! @return true if client can access diagnostic data, false otherwise. If call return false
//! here, BLE service will return ATT_ERROR_READ_NOT_PERMITTED / ATT_ERROR_WRITE_NOT_PERMITTED
//! for any read or write operation to the GATT service, respectively
extern bool mds_access_enabled(uint16_t connection_handle);


#ifdef __cplusplus
}
#endif
