#pragma once

//! @file
//!
//! Contains Memfault SDK version information.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
} sMfltSdkVersion;

#define MEMFAULT_SDK_VERSION   { .major = 0, .minor = 24, .patch = 2 }

#ifdef __cplusplus
}
#endif
