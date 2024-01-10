#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_build_id.h"
#include "memfault/core/platform/device_info.h"
#include "memfault_self_test_private.h"

#define VALID_DEVICE_SERIAL "device_serial-"
#define VALID_SOFTWARE_TYPE "1.0_+:software-type"
#define VALID_SOFTWARE_VERSION "1.2.3+dev"
#define VALID_HARDWARE_VERSION "1.0_+:hw-version"

extern "C" {

void memfault_platform_get_device_info(sMemfaultDeviceInfo *device_info) {
  mock().actualCall(__func__).withOutputParameter("device_info", device_info);
}
}

typedef struct {
  const char *invalid_str;
  const char *valid_str;
  uint32_t invalid_result;
} sDeviceInfoFieldParam;

static const char *invalid_long_field =
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static sMemfaultDeviceInfo s_device_info = { 0 };

static void prv_set_device_info_valid(void) {
  s_device_info.device_serial = VALID_DEVICE_SERIAL;
  s_device_info.software_type = VALID_SOFTWARE_TYPE;
  s_device_info.software_version = VALID_SOFTWARE_VERSION;
  s_device_info.hardware_version = VALID_HARDWARE_VERSION;
}

static void prv_test_device_info_helper(long expected_val) {
  mock()
    .expectOneCall("memfault_platform_get_device_info")
    .withOutputParameterReturning("device_info", &s_device_info, sizeof(sMemfaultDeviceInfo));

  uint32_t result = memfault_self_test_device_info_test();
  LONGS_EQUAL(expected_val, result);
}

static void prv_test_device_info_field_helper(const char **field, sDeviceInfoFieldParam *params) {
  // Test NULL
  *field = NULL;
  prv_test_device_info_helper(params->invalid_result);

  // Test short
  *field = "";
  prv_test_device_info_helper(params->invalid_result);

  // Test long
  *field = invalid_long_field;
  prv_test_device_info_helper(params->invalid_result);

  // Test invalid string
  *field = params->invalid_str;
  prv_test_device_info_helper(params->invalid_result);

  // Test valid string
  *field = params->valid_str;
  prv_test_device_info_helper(0);
}

TEST_GROUP(MemfaultSelfTestDeviceInfo) {
  void setup() {
    prv_set_device_info_valid();
    g_fake_memfault_build_id_type = kMemfaultBuildIdType_MemfaultBuildIdSha1;
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
    fake_memfault_build_id_reset();
  }
};

TEST(MemfaultSelfTestDeviceInfo, Test_ValidateDeviceSerial) {
  sDeviceInfoFieldParam params = {
    .invalid_str = "invalid+",
    .valid_str = VALID_DEVICE_SERIAL,
    .invalid_result = 0x1,
  };

  prv_test_device_info_field_helper(&s_device_info.device_serial, &params);
}

TEST(MemfaultSelfTestDeviceInfo, Test_ValidateSoftwareType) {
  sDeviceInfoFieldParam params = {
    .invalid_str = "invalid{}",
    .valid_str = VALID_SOFTWARE_TYPE,
    .invalid_result = 0x2,
  };

  prv_test_device_info_field_helper(&s_device_info.software_type, &params);
}

TEST(MemfaultSelfTestDeviceInfo, Test_ValidateSoftwareVersion) {
  sDeviceInfoFieldParam params = {
    .invalid_str = "invalid\a",
    .valid_str = VALID_SOFTWARE_VERSION,
    .invalid_result = 0x4,
  };

  prv_test_device_info_field_helper(&s_device_info.software_version, &params);
}

TEST(MemfaultSelfTestDeviceInfo, Test_ValidateHardwareVersion) {
  sDeviceInfoFieldParam params = {
    .invalid_str = "invalid{}",
    .valid_str = VALID_HARDWARE_VERSION,
    .invalid_result = 0x8,
  };

  prv_test_device_info_field_helper(&s_device_info.hardware_version, &params);
}

TEST(MemfaultSelfTestDeviceInfo, Test_ValidateBuildId) {
  // Explicitly clear the build ID info, test invalid build ID
  g_fake_memfault_build_id_type = kMemfaultBuildIdType_None;

  mock()
    .expectOneCall("memfault_platform_get_device_info")
    .withOutputParameterReturning("device_info", &s_device_info, sizeof(sMemfaultDeviceInfo));
  uint32_t result = memfault_self_test_device_info_test();
  LONGS_EQUAL(0x10, result);

  // Test a valid build ID
  g_fake_memfault_build_id_type = kMemfaultBuildIdType_MemfaultBuildIdSha1;

  mock()
    .expectOneCall("memfault_platform_get_device_info")
    .withOutputParameterReturning("device_info", &s_device_info, sizeof(sMemfaultDeviceInfo));
  result = memfault_self_test_device_info_test();
  LONGS_EQUAL(0, result);
}
