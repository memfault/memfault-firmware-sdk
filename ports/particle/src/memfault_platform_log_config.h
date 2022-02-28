#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Port of the Memfault logging APIs to the particle subsystem

#ifdef __cplusplus
extern "C" {
#endif

#include "logging.h"
#include "memfault-firmware-sdk/components/include/memfault/config.h"

#if MEMFAULT_PARTICLE_PORT_LOGGING_ENABLE

#define MEMFAULT_LOG_DEBUG(...) LOG_DEBUG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define MEMFAULT_LOG_INFO(...) LOG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define MEMFAULT_LOG_WARN(...) LOG_C(WARN, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define MEMFAULT_LOG_ERROR(...) LOG_C(ERROR, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define MEMFAULT_LOG_RAW(...) LOG_DEBUG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)

#else

#define MEMFAULT_LOG_DEBUG(...)
#define MEMFAULT_LOG_INFO(...)
#define MEMFAULT_LOG_WARN(...)
#define MEMFAULT_LOG_ERROR(...)
#define MEMFAULT_LOG_RAW(...)

#endif

#ifdef __cplusplus
}
#endif
