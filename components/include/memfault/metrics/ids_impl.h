#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! NOTE: The internals of the metric APIs make use of "X-Macros" to enable more flexibility
//! improving and extending the internal implementation without impacting the externally facing API

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/config.h"

#define MEMFAULT_METRICS_SESSION_TIMER_NAME(key_name) mflt_session_timer_##key_name

//! Generate extern const char * declarations for all IDs (used in key names):
#define MEMFAULT_METRICS_KEY_DEFINE_(key_name) \
  extern const char* const g_memfault_metrics_id_##key_name;

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, _min, _max) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, _)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, _)

#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name) \
  MEMFAULT_METRICS_KEY_DEFINE(MEMFAULT_METRICS_SESSION_TIMER_NAME(key_name), _)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)         \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) MEMFAULT_METRICS_KEY_DEFINE_(key_name)
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_

#define MEMFAULT_METRICS_KEY_DEFINE_(key_name) kMfltMetricsIndex_##key_name,

//! Generate an enum for all IDs (used for indexing into values)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, _)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, _)

#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name) \
  MEMFAULT_METRICS_KEY_DEFINE(MEMFAULT_METRICS_SESSION_TIMER_NAME(key_name), _)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)         \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) MEMFAULT_METRICS_KEY_DEFINE_(key_name)

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
} eMfltMetricsIndex;

#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name) kMfltMetricsSessionKey_##key_name,
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_key)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)

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

#define _MEMFAULT_METRICS_ID_CREATE(id) \
  { kMfltMetricsIndex_##id }

#define _MEMFAULT_METRICS_ID(id) ((MemfaultMetricId){kMfltMetricsIndex_##id})

#ifdef __cplusplus
}
#endif
