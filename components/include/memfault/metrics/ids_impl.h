#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! NOTE: The internals of the metric APIs make use of "X-Macros" to enable more flexibility
//! improving and extending the internal implementation without impacting the externally facing API

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/config.h"

// Clear any potential issues from transitive dependencies in these files by
// including them one time, with stubs for the macros we need to define. This
// set up any multiple-include guards, and we can safely include the x-macro
// definitions later.

#define MEMFAULT_METRICS_KEY_DEFINE(...)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(...)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(...)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(...)
#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(...)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(...)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(...)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(...)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE(...)

#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE

#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE

#define MEMFAULT_METRICS_KEY_DEFINE_(session_name, key_name) \
  kMfltMetricsIndex_##session_name##__##key_name,

//! Generate an enum for all IDs (used for indexing into values)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, _)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE_(session_name, key_name)

//! Sessions have the following built-in keys:
//! - "<session name>__MemfaultSdkMetric_IntervalMs"
//! - "<session name>__operational_crashes"
#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(session_name)             \
  kMfltMetricsIndex_##session_name##__##MemfaultSdkMetric_IntervalMs, \
    kMfltMetricsIndex_##session_name##__##operational_crashes,

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE_(session_name, key_name)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)         \
  MEMFAULT_METRICS_KEY_DEFINE_(session_name, key_name)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(key_name, value_type, scale_value) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE(key_name, value_type,     \
                                                                 session_key, scale_value) \
  MEMFAULT_METRICS_KEY_DEFINE_(session_key, key_name)

#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
  MEMFAULT_METRICS_KEY_DEFINE_(heartbeat, key_name)

typedef enum MfltMetricsIndex {
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE
} eMfltMetricsIndexV2;
typedef eMfltMetricsIndexV2 eMfltMetricsIndex;

#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name) kMfltMetricsSessionKey_##key_name,
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(key_name, value_type, scale_value)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_key)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE(key_name, value_type, \
                                                                 session_key, scale_value)

typedef enum MfltMetricSessionIndex {
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  MEMFAULT_METRICS_SESSION_KEY_DEFINE(heartbeat)
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE
} eMfltMetricsSessionIndex;

//! Stub define to detect accidental usage outside of the heartbeat config files
#define MEMFAULT_METRICS_KEY_DEFINE_TRAP_()                                        \
  MEMFAULT_STATIC_ASSERT(false, "MEMFAULT_METRICS_KEY_DEFINE should only be used " \
                                "in " MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE);

//! NOTE: Access to a key should _always_ be made via the MEMFAULT_METRICS_KEY() macro to ensure
//! source code compatibility with future APIs updates
//!
//! The struct wrapper does not have any function, except for preventing one from passing a C
//! string to the API:
typedef struct {
  int _impl;
} MemfaultMetricId;

#define MEMFAULT_METRICS_SESSION_KEY(key_name) kMfltMetricsSessionKey_##key_name

#define _MEMFAULT_METRICS_ID_CREATE(id, session_name) { kMfltMetricsIndex_##session_name##__##id }

#define _MEMFAULT_METRICS_ID(id, session_name) \
  ((MemfaultMetricId){ kMfltMetricsIndex_##session_name##__##id })

#ifdef __cplusplus
}
#endif
