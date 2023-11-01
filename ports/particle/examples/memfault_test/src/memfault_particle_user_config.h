#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Application specific configuration overrides for memfault library

#ifdef __cplusplus
extern "C" {
#endif

//! WARNING: This should only be set for debug purposes. For production fleets, the
//! value must be >= 3600 to avoid being rate limited
#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 60

//! This exposes debug commands that can be called for testing Memfault at the cost of using
//! some extra code space. For production builds, it is recommended this flag be set to 0
#define MEMFAULT_PARTICLE_PORT_DEBUG_API_ENABLE 1

//! The software_type name to be displayed in the Memfault UI
#define MEMFAULT_PARTICLE_PORT_SOFTWARE_TYPE "mflt-test-fw"

//! The particle librt-dynalib.a library implements a custom __assert_func(), so
//! disable the conflicting Memfault SDK implementation
#define MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED 0

#ifdef __cplusplus
}
#endif
