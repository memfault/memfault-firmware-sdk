//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "memfault/core/serializer_helper.h"

#include "memfault/core/platform/device_info.h"
#include "memfault/core/serializer_key_ids.h"
#include "memfault/util/cbor.h"

static bool prv_encode_event_key_string_pair(
    sMemfaultCborEncoder *encoder, eMemfaultEventKey key,  const char *value) {
  return memfault_cbor_encode_unsigned_integer(encoder, key) &&
      memfault_cbor_encode_string(encoder, value);
}

static bool prv_encode_device_version_info(sMemfaultCborEncoder *e) {
  // Encoding something like:
  //
  // "device_serial": "ABCD1234",
  // "software_type": "main-fw",
  // "software_version": "1.0.0",
  // "hardware_version": "hwrev1",
  //
  // NOTE: int keys are used instead of strings to minimize the wire payload.

  sMemfaultDeviceInfo info = { 0 };
  memfault_platform_get_device_info(&info);

  if (!prv_encode_event_key_string_pair(e, kMemfaultEventKey_DeviceSerial, info.device_serial)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kMemfaultEventKey_SoftwareType, info.software_type)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kMemfaultEventKey_SoftwareVersion, info.software_version)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kMemfaultEventKey_HardwareVersion, info.hardware_version)) {
    return false;
  }

  return true;
}

bool memfault_serializer_helper_encode_version_info(sMemfaultCborEncoder *encoder) {
  if (!memfault_serializer_helper_encode_uint32_kv_pair(
          encoder, kMemfaultEventKey_CborSchemaVersion, MEMFAULT_CBOR_SCHEMA_VERSION_V1)) {
    return false;
  }

  return prv_encode_device_version_info(encoder);
}

bool memfault_serializer_helper_encode_uint32_kv_pair(
    sMemfaultCborEncoder *encoder, uint32_t key, uint32_t value) {
  return memfault_cbor_encode_unsigned_integer(encoder, key) &&
      memfault_cbor_encode_unsigned_integer(encoder, value);
}
