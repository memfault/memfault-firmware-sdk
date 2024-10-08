#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Utilities used within the "demo" component.

#ifdef __cplusplus
extern "C" {
#endif

//! Returns the URL to the chunks endpoint
//!
//! @note A non-weak implementation is exposed in demo/src/http/memfault_demo_https.c and picked up
//!   automatically by the build system helpers when the "http" component is enabled.
const char *memfault_demo_get_chunks_url(void);

//! Returns the current Project Key
//!
//! @note A non-weak implementation is exposed in demo/src/http/memfault_demo_https.c and picked up
//!   automatically by the build system helpers when the "http" component is enabled.
const char *memfault_demo_get_api_project_key(void);

#ifdef __cplusplus
}
#endif
