//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/sdk_assert.h"
#include "memfault/core/self_test.h"
#include "memfault_self_test_private.h"

bool memfault_self_test_valid_device_serial(unsigned char c) {
  return (isalnum(c) > 0) || (c == '_') || (c == '-');
}

bool memfault_self_test_valid_hw_version_sw_type(unsigned char c) {
  return (memfault_self_test_valid_device_serial(c)) || (c == '.') || (c == '+') || (c == ':');
}

bool memfault_self_test_valid_sw_version(unsigned char c) {
  return (isprint(c) > 0);
}

static const struct {
  char *name;
  uint32_t value;
} s_test_flags[] = {
  {
    .name = "reboot",
    .value = kMemfaultSelfTestFlag_RebootReason,
  },
  {
    .name = "reboot_verify",
    .value = kMemfaultSelfTestFlag_RebootReasonVerify,
  },
  {
    .name = "coredump_storage",
    .value = kMemfaultSelfTestFlag_CoredumpStorage,
  },
};

#define SELF_TEST_MAX_NAME_LEN 30

uint32_t memfault_self_test_arg_to_flag(const char *arg) {
  MEMFAULT_SDK_ASSERT(arg != NULL);
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_test_flags); i++) {
    if (strncmp(arg, s_test_flags[i].name, SELF_TEST_MAX_NAME_LEN) == 0) {
      return (uint32_t)s_test_flags[i].value;
    }
  }

  MEMFAULT_LOG_WARN("No test type found for arg: %s. Default tests selected", arg);
  return kMemfaultSelfTestFlag_Default;
}

// Include an implementation of strnlen, based on picolibc/newlib
// For reference: https://github.com/picolibc/picolibc/blob/main/newlib/libc/string/strnlen.c
size_t memfault_strnlen(const char *str, size_t n) {
  size_t count = 0;

  // Must check n first to prevent out of bounds access
  while (n-- && *str) {
    count++;
    str++;
  }

  return count;
}

MEMFAULT_WEAK void memfault_self_test_platform_delay(MEMFAULT_UNUSED uint32_t delay_ms) {
  MEMFAULT_LOG_ERROR("%s not implemented for this platform", __func__);
}

MEMFAULT_WEAK bool memfault_self_test_platform_disable_irqs(void) {
  MEMFAULT_LOG_ERROR("%s not implemented for this platform", __func__);
  return false;
}

MEMFAULT_WEAK bool memfault_self_test_platform_enable_irqs(void) {
  MEMFAULT_LOG_ERROR("%s not implemented for this platform", __func__);
  return false;
}
