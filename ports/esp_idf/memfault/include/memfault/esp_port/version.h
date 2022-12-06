#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Provide some esp-idf version identifiers that work across esp-idf versions.
//!

#include "esp_idf_version.h"

#ifdef __cplusplus
extern "C" {
#endif

// These version macros were added in esp-idf v4.0. If they don't exist, it's an
// earlier release. Define the macros so they can be used, and set a fallback
// version of 3.99.99 for comparison purposes. These macros can't be used to
// compare for earlier versions.

#if !defined(ESP_IDF_VERSION)

#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

#define ESP_IDF_VERSION ESP_IDF_VERSION_VAL(3, 99, 99)

#endif

#ifdef __cplusplus
}
#endif
