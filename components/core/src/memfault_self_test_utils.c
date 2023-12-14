//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#include <ctype.h>
#include <stdbool.h>

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
