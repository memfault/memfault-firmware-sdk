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

#include <stdint.h>

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
#undef MEMFAULT_METRICS_KEY_DEFINE_
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
  kMfltMetricsIndexV2_COUNT,
} eMfltMetricsIndexV2;
typedef eMfltMetricsIndexV2 eMfltMetricsIndex;

//! Count the number of user-defined timer metrics, for computing context size
//! lower down.
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_
#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_key)
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Unsigned(_name)
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Signed(_name)
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Timer(_name) +1
#define MEMFAULT_METRICS_KEY_DEFINE(_name, _type) MEMFAULT_METRICS_STATE_HELPER_##_type(_name)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(_name, _type, _session_name) \
  MEMFAULT_METRICS_STATE_HELPER_##_type(_name)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE(_name, _type, _session_name, \
                                                                 _scale_value)                \
  MEMFAULT_METRICS_STATE_HELPER_##_type(_name)

extern uint8_t g_memfault_metrics_timer_count_unused[0
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE
];

#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE

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
    kMfltMetricsSessionKey_COUNT,
} eMfltMetricsSessionIndex;

//! Compute the total size of string key metric storage, by tallying up the
//! individual string metric sizes. Use a uint8_t array to compute the size,
//! because we can't x-macro within a #define. Set it to 1 byte to account for
//! the placeholder byte.
#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name)
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) +(max_length + 1)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE(key_name, value_type, scale_value)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_key) \
  +(max_length + 1)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_name)
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_name)

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE(key_name, value_type, \
                                                                 session_key, scale_value)

extern uint8_t g_memfault_metrics_string_storage[1
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_SESSION_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SCALE_VALUE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION_AND_SCALE_VALUE
];

// MEMFAULT_IS_SET_FLAGS_PER_BYTE must be a power of 2
// MEMFAULT_IS_SET_FLAGS_DIVIDER must be equal to log2(MEMFAULT_IS_SET_FLAGS_PER_BYTE)
#define MEMFAULT_IS_SET_FLAGS_PER_BYTE 8
#define MEMFAULT_IS_SET_FLAGS_DIVIDER 3

#if MEMFAULT_METRICS_LOGS_ENABLE
  #define MEMFAULT_METRICS_CONTEXT_LOG_SIZE_BYTES \
    /* last log counts */                         \
    2 * sizeof(uint32_t)
#else
  #define MEMFAULT_METRICS_CONTEXT_LOG_SIZE_BYTES 0
#endif

//! Compute the amount of space needed to store the metrics context structure,
//! used for backing up across deep sleep. Tally each structure member size
//! and add them up.

//! 1. Sessions: each session implements:
//!    - a start callback
//!    - an end callback
//!    - a timer + operational crashes metric (these are counted in the metrics count later)
//! 2. Metrics: each metric needs:
//!    - space for its current value
//!    - a bit in the "is set" bitmap (needs to include padding for alignment)
//! 3. Timer metrics: need an additional metadata storage
//!   - 4 bytes
//! 4. String metrics: need additional storage for the string value
//!   - user-specified size for each metric
//!   - padding for alignment
//! 5. 2 * 4 byte last log counts for log metrics (optional)

// Intentionally not using MEMFAULT_MAX()/MEMFAULT_CEIL_DIV() here, to avoid
// bringing in extra includes
#define MEMFAULT_METRICS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MEMFAULT_METRICS_CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define MEMFAULT_METRICS_CONTEXT_METRIC_SIZE MEMFAULT_METRICS_MAX(sizeof(uint32_t), sizeof(void *))
#if (INTPTR_MAX == 0x7fffffff)
  #define MEMFAULT_METRICS_CONTEXT_PAD_SIZE 0
#elif (INTPTR_MAX == 0x7fffffffffffffff)
  #define MEMFAULT_METRICS_CONTEXT_PAD_SIZE 4
#endif

// clang-format off
#define MEMFAULT_METRICS_CONTEXT_SIZE_BYTES \
  /* context pointer */ \
  sizeof(void *) + \
  /* reliability context */ \
  sizeof(uint32_t) * 2 + sizeof(bool) + /* padding */ 4 - sizeof(bool) + MEMFAULT_METRICS_CONTEXT_PAD_SIZE + \
  /* session start and end callbacks */ \
  kMfltMetricsSessionKey_COUNT * sizeof(MemfaultMetricsSessionStartCb) + \
  kMfltMetricsSessionKey_COUNT * sizeof(MemfaultMetricsSessionEndCb) + \
  /* all the heartbeat metric values */ \
  kMfltMetricsIndexV2_COUNT * MEMFAULT_METRICS_CONTEXT_METRIC_SIZE + \
  /* the "is set" bitmask */ \
  MEMFAULT_METRICS_CEIL_DIV( \
    kMfltMetricsIndexV2_COUNT, MEMFAULT_IS_SET_FLAGS_PER_BYTE) + \
    /* padding of "is set" for 4-byte alignment */ \
    (sizeof(uint32_t) - (MEMFAULT_METRICS_CEIL_DIV( \
      kMfltMetricsIndexV2_COUNT, MEMFAULT_IS_SET_FLAGS_PER_BYTE) % sizeof(uint32_t))) % sizeof(uint32_t) + \
    /* timer metrics metadata. include session count -1, since heartbeat is built-in */ \
    (sizeof(g_memfault_metrics_timer_count_unused) + kMfltMetricsSessionKey_COUNT - 1) * \
    sizeof(uint32_t) + \
  /* string metrics storage */ \
  sizeof(g_memfault_metrics_string_storage) + \
  /* padding for alignment */ \
  (sizeof(uint32_t) - ((sizeof(g_memfault_metrics_string_storage)) % sizeof(uint32_t))) % sizeof(uint32_t) + \
  /* log metrics */ \
  MEMFAULT_METRICS_CONTEXT_LOG_SIZE_BYTES
// clang-format on

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
