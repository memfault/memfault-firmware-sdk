//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault SDK self test functions to verify SDK integration

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "memfault/core/build_info.h"
#include "memfault/core/data_export.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/core/self_test.h"
#include "memfault/core/trace_event.h"
#include "memfault/panics/coredump.h"
#include "memfault/panics/coredump_impl.h"
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
  char build_id_str[(MEMFAULT_BUILD_ID_LEN * 2) + 1] = { 0 };
  uint32_t result = 0;

  if (!memfault_build_id_get_string(build_id_str, MEMFAULT_ARRAY_SIZE(build_id_str))) {
    result = (uint32_t)(1 << kDeviceInfoField_BuildId);
  }

  return result;
}

static void prv_device_info_test_describe(uint32_t results) {
  MEMFAULT_SELF_TEST_PRINT_HEADER("Device Info Test");

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
  MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_END_OUTPUT);
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

typedef enum MfltBootComponent {
  kMfltBootComponent_EventStorage = 0,
  kMfltBootComponent_Logging,
  kMfltBootComponent_RebootTracking,
  kMfltBootComponent_TraceEvent,
  kMfltBootComponent_NumTypes,
} eMfltBootComponent;

static const struct {
  char *component_name;
  bool (*booted)(void);
} s_boot_components[kMfltBootComponent_NumTypes] = {
  [kMfltBootComponent_EventStorage] =
    {
      .component_name = "Event Storage",
      memfault_event_storage_booted,
    },
  [kMfltBootComponent_Logging] =
    {
      .component_name = "Logging",
      memfault_log_booted,
    },
  [kMfltBootComponent_RebootTracking] =
    {
      .component_name = "Reboot Tracking",
      memfault_reboot_tracking_booted,
    },
  [kMfltBootComponent_TraceEvent] =
    {
      .component_name = "Trace Event",
      memfault_trace_event_booted,
    },
};

void memfault_self_test_component_boot_test(void) {
  MEMFAULT_SELF_TEST_PRINT_HEADER("Component Boot Test");
  MEMFAULT_LOG_INFO("%-16s|%8s|", "Component", "Booted?");
  MEMFAULT_LOG_INFO("-----------------------------");
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_boot_components); i++) {
    bool booted = s_boot_components[i].booted();
    MEMFAULT_LOG_INFO("%-16s|%8s|", s_boot_components[i].component_name, booted ? "yes" : "no");
  }
  MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_END_OUTPUT);
}

void memfault_self_test_data_export_test(void) {
  char test_line[MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN] = { 0 };
  // Set every char to '+' except for the last char before null terminator
  memset(test_line, '+', MEMFAULT_ARRAY_SIZE(test_line) - 2);
  // Last character before null should be '1'
  test_line[MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN - 2] = '1';

  MEMFAULT_SELF_TEST_PRINT_HEADER("Data Export Line Test");
  MEMFAULT_LOG_INFO("Printing %u characters, confirm line ends with '1' and is not split",
                    (unsigned)(MEMFAULT_DATA_EXPORT_BASE64_CHUNK_MAX_LEN - 1));
  MEMFAULT_LOG_INFO("%s", test_line);
  MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_END_OUTPUT);
}

static void prv_print_region_group_info(const char *group_name, const sMfltCoredumpRegion *regions,
                                        const size_t num_regions) {
  MEMFAULT_LOG_INFO("Coredump group: %s", group_name);
  MEMFAULT_LOG_INFO("-----------------------------");
  MEMFAULT_LOG_INFO("%10s|%10s|%6s|", "Address", "Length", "Type");
  MEMFAULT_LOG_INFO("-----------------------------");
  for (size_t i = 0; i < num_regions; i++) {
    sMfltCoredumpRegion region = regions[i];
    MEMFAULT_LOG_INFO("0x%08" PRIxPTR "|%10" PRIu32 "|%6u|", (uintptr_t)region.region_start,
                      region.region_size, region.type);
  }
  MEMFAULT_LOG_INFO("-----------------------------");
}

uint32_t memfault_self_test_coredump_regions_test(void) {
  uint32_t result = 0;
  const sMfltCoredumpRegion *platform_regions = NULL;
  size_t num_platform_regions = 0;

  // Set stack address to current sp and unknown reboot reason
  // These are dummy values so we can get as close as possible to the regions
  // that memfault_platform_coredump_get_regions will return
  sCoredumpCrashInfo info = {
    .stack_address = &num_platform_regions,
    .trace_reason = kMfltRebootReason_Unknown,
  };

  // Call memfault_platform_coredump_get_regions
  platform_regions = memfault_platform_coredump_get_regions(&info, &num_platform_regions);

  // Get arch and SDK regions
  size_t num_arch_regions = 0;
  const sMfltCoredumpRegion *arch_regions = memfault_coredump_get_arch_regions(&num_arch_regions);
  size_t num_sdk_regions = 0;
  const sMfltCoredumpRegion *sdk_regions = memfault_coredump_get_sdk_regions(&num_sdk_regions);

  MEMFAULT_SELF_TEST_PRINT_HEADER("Coredump Regions Test");
  if (num_platform_regions == 0) {
    MEMFAULT_LOG_ERROR("Number of platform regions = 0");
    result = 1;
  }

  if (platform_regions == NULL) {
    MEMFAULT_LOG_ERROR("Platform regions was NULL");
    result = 1;
  }

  prv_print_region_group_info("Platform Regions", platform_regions, num_platform_regions);
  prv_print_region_group_info("Arch Regions", arch_regions, num_arch_regions);
  prv_print_region_group_info("SDK Regions", sdk_regions, num_sdk_regions);
  MEMFAULT_LOG_INFO(MEMFAULT_SELF_TEST_END_OUTPUT);
  return result;
}

int memfault_self_test_run(void) {
  // Run each test one at a time and return result of all runs OR'd together
  memfault_self_test_component_boot_test();
  memfault_self_test_data_export_test();
  memfault_self_test_coredump_regions_test();
  return (memfault_self_test_device_info_test() != 0);
}
