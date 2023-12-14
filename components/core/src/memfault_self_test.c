//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault SDK self test functions to verify SDK integration
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "memfault/core/build_info.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/self_test.h"
#include "memfault_reboot_tracking_private.h"
#include "memfault_self_test_private.h"

typedef enum {
  kDeviceInfoField_DeviceSerial = 0,
  kDeviceInfoField_SoftwareType,
  kDeviceInfoField_SoftwareVersion,
  kDeviceInfoField_HardwareVersion,
  kDeviceInfoField_BuildId,
  kDeviceInfoField_Max,
} eDeviceInfoField;

static const char *s_device_info_field_names[] = {
  [kDeviceInfoField_DeviceSerial] = "Device Serial",
  [kDeviceInfoField_SoftwareType] = "Software Type",
  [kDeviceInfoField_SoftwareVersion] = "Software Version",
  [kDeviceInfoField_HardwareVersion] = "Hardware Version",
  [kDeviceInfoField_BuildId] = "Build ID",
};

typedef bool (*FieldCharValidator)(unsigned char c);

static const FieldCharValidator s_device_info_str_validators[] = {
  [kDeviceInfoField_DeviceSerial] = memfault_self_test_valid_device_serial,
  [kDeviceInfoField_SoftwareType] = memfault_self_test_valid_hw_version_sw_type,
  [kDeviceInfoField_SoftwareVersion] = memfault_self_test_valid_sw_version,
  [kDeviceInfoField_HardwareVersion] = memfault_self_test_valid_hw_version_sw_type,
  NULL,
};

MEMFAULT_STATIC_ASSERT(MEMFAULT_ARRAY_SIZE(s_device_info_field_names) == kDeviceInfoField_Max,
                       "Mismatch in field name table, must be equal");
MEMFAULT_STATIC_ASSERT(MEMFAULT_ARRAY_SIZE(s_device_info_str_validators) == kDeviceInfoField_Max,
                       "Mismatch in string validator table, must be equal");

static bool prv_validate_string(const char *str, size_t len, FieldCharValidator is_valid) {
  for (size_t idx = 0; idx < len; ++idx) {
    unsigned char c = (unsigned char)str[idx];
    if (!is_valid(c)) {
      MEMFAULT_LOG_ERROR("Invalid char %c, found in %s", c, str);
      return false;
    }
  }
  return true;
}

static bool is_field_valid(const char *str, eDeviceInfoField field) {
  if (str == NULL) {
    return false;
  }
  // First validate min and max length
  // Max + 1 needed determine if we exceeded bounds before NULL found
  size_t len = strnlen(str, MEMFAULT_DEVICE_INFO_MAX_STRING_SIZE + 1);
  if ((len < 1) || (len > MEMFAULT_DEVICE_INFO_MAX_STRING_SIZE)) {
    MEMFAULT_LOG_ERROR("Invalid length %zu for %s", len, s_device_info_field_names[field]);
    return false;
  }

  if (field < kDeviceInfoField_Max) {
    FieldCharValidator validator = s_device_info_str_validators[field];
    return (validator != NULL) ? prv_validate_string(str, len, validator) : false;
  } else {
    MEMFAULT_LOG_ERROR("Invalid device info string type %u for %s", field, str);
    return false;
  }
}

static uint32_t prv_validate_device_info_field(const char *str, eDeviceInfoField field) {
  uint32_t result = 0;
  if (!is_field_valid(str, field)) {
    result = (uint32_t)(1 << field);
  }

  return result;
}

static uint32_t prv_validate_device_info(void) {
  sMemfaultDeviceInfo device_info = { 0 };
  memfault_platform_get_device_info(&device_info);
  uint32_t results = 0;

  // Validate each field in device_info
  results |=
    prv_validate_device_info_field(device_info.device_serial, kDeviceInfoField_DeviceSerial);

  results |=
    prv_validate_device_info_field(device_info.hardware_version, kDeviceInfoField_HardwareVersion);

  results |=
    prv_validate_device_info_field(device_info.software_version, kDeviceInfoField_SoftwareVersion);

  results |=
    prv_validate_device_info_field(device_info.software_type, kDeviceInfoField_SoftwareType);

  return results;
}

static uint32_t prv_validate_build_id(void) {
  char build_id_str[(MEMFAULT_BUILD_ID_LEN * 2) + 1] = {0};
  uint32_t result = 0;

  if (!memfault_build_id_get_string(build_id_str, MEMFAULT_ARRAY_SIZE(build_id_str))) {
    result = (uint32_t)(1 << kDeviceInfoField_BuildId);
  }

  return result;
}

static void prv_device_info_test_describe(uint32_t results) {
  MEMFAULT_LOG_INFO("Device Info Test Results");
  MEMFAULT_LOG_INFO("------------------------");

  if (results == 0) {
    MEMFAULT_LOG_INFO("All fields valid");
    return;
  }

  MEMFAULT_LOG_ERROR("One or more fields is invalid. Check for correct length and contents");
  for (uint8_t i = 0; i < kDeviceInfoField_Max; i++) {
    // Check if bit cleared, cleared bits indicate an invalid field
    if ((results & (1 << i))) {
      MEMFAULT_LOG_ERROR("%s invalid", s_device_info_field_names[i]);
    }
  }
}

uint32_t memfault_self_test_device_info_test(void) {
  uint32_t results = 0;
  // Validate the build ID
  results |= prv_validate_build_id();
  // Valid device info fields
  results |= prv_validate_device_info();

  prv_device_info_test_describe(results);
  return results;
}

int memfault_self_test_run(void) {
  // Run each test one at a time and return result of all runs OR'd together
  return (memfault_self_test_device_info_test() != 0);
}
