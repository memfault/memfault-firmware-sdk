//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault metrics API implementation. See header for more details.

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/core/platform/overrides.h"
#include "memfault/core/serializer_helper.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/overrides.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/metrics/serializer.h"
#include "memfault/metrics/utils.h"

//! Disable this warning; it trips when there's no custom macros defined of a
//! given type
MEMFAULT_DISABLE_WARNING("-Wunused-macros")

#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE

#define MEMFAULT_METRICS_KEY_NOT_FOUND (-1)
#define MEMFAULT_METRICS_TYPE_INCOMPATIBLE (-2)
#define MEMFAULT_METRICS_TYPE_BAD_PARAM (-3)
#define MEMFAULT_METRICS_TYPE_NO_CHANGE (-4)
#define MEMFAULT_METRICS_STORAGE_TOO_SMALL (-5)
#define MEMFAULT_METRICS_TIMER_BOOT_FAILED (-6)

typedef struct MemfaultMetricKVPair {
  MemfaultMetricId key;
  eMemfaultMetricType type;
  // Notes:
  // - We treat 'min' as a _signed_ integer when the 'type' == kMemfaultMetricType_Signed
  // - We parse this range information in the Memfault cloud to better normalize data presented in
  //   the UI.
  uint32_t min;
  uint32_t range;
} sMemfaultMetricKVPair;

// Generate heartbeat keys table (ROM):
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  { .key = _MEMFAULT_METRICS_ID_CREATE(key_name), .type = value_type, \
    .min = (uint32_t)min_value, .range = ((int64_t)max_value - (int64_t)min_value) },

#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
  MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, 0, 0)

// Store the string max length in the range field (excluding the null terminator)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, kMemfaultMetricType_String, 0, max_length)

static const sMemfaultMetricKVPair s_memfault_heartbeat_keys[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};

#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, _min, _max) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)

// Generate global ID constants (ROM):
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)         \
  const char * const g_memfault_metrics_id_##key_name = MEMFAULT_EXPAND_AND_QUOTE(key_name);

#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE

MEMFAULT_STATIC_ASSERT(MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys) != 0,
                       "At least one \"MEMFAULT_METRICS_KEY_DEFINE\" must be defined");

#define MEMFAULT_METRICS_TIMER_VAL_MAX 0x80000000
typedef struct MemfaultMetricValueMetadata {
  bool is_running:1;
  // We'll use 32 bits since the rollover time is ~25 days which is much much greater than a
  // reasonable heartbeat interval. This let's us track whether or not the timer is running in the
  // top bit
  uint32_t start_time_ms:31;
} sMemfaultMetricValueMetadata;

typedef struct MemfaultMetricValueInfo {
  union MemfaultMetricValue *valuep;
  sMemfaultMetricValueMetadata *meta_datap;
} sMemfaultMetricValueInfo;


// Allocate storage for string values- additional byte for null terminator.
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  static char g_memfault_metrics_string_##key_name[max_length + 1 /* for NUL */];
#include "memfault/metrics/heartbeat_config.def"
#include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef MEMFAULT_METRICS_KEY_DEFINE
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE

// Generate a mapping of key index to key value position in s_memfault_heartbeat_values.
// First produce a sparse enum for the key values that are stored in s_memfault_heartbeat_values.
typedef enum MfltMetricKeyToValueIndex {
  #define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
    kMfltMetricKeyToValueIndex_##key_name,
  #define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)

  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
  kMfltMetricKeyToValueIndex_Count
} eMfltMetricKeyToValueIndex;
// Now generate a table mapping the canonical key ID to the index in s_memfault_heartbeat_values
static const eMfltMetricKeyToValueIndex s_memfault_heartbeat_key_to_valueindex[] = {
  #define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
    kMfltMetricKeyToValueIndex_##key_name,
  #define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    0,  // 0 for the placeholder so it's safe to index with

  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};
MEMFAULT_STATIC_ASSERT(MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys) == MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_key_to_valueindex),
                       "Mismatch between s_memfault_heartbeat_keys and s_memfault_heartbeat_key_to_valueindex");
// And a similar approach for the strings
typedef enum MfltMetricStringKeyToIndex {
  #define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
  #define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    kMfltMetricStringKeyToIndex_##key_name,

  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
  kMfltMetricStringKeyToIndex_Count
} eMfltMetricStringKeyToIndex;
// Now generate a table mapping the canonical key ID to the index in s_memfault_heartbeat_values
static const eMfltMetricStringKeyToIndex s_memfault_heartbeat_string_key_to_index[] = {
  #define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
    0,  // 0 for the placeholder so it's safe to index with
  #define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    kMfltMetricStringKeyToIndex_##key_name,

  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};
MEMFAULT_STATIC_ASSERT(MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys) == MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_string_key_to_index),
                       "Mismatch between s_memfault_heartbeat_keys and s_memfault_heartbeat_string_key_to_index");

// Generate heartbeat values table (RAM), sparsely populated: only for the scalar types
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) { 0 },
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)
static union MemfaultMetricValue s_memfault_heartbeat_values[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};

// String value lookup table. Const- the pointers do not change at runtime, so
// this table can be stored in ROM and save a little RAM.
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) { .ptr = g_memfault_metrics_string_##key_name },
static const union MemfaultMetricValue s_memfault_heartbeat_string_values[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
  // include a stub entry to prevent compilation errors when no strings are defined
  {.ptr = NULL},
};

// Timer metadata table
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Unsigned(_name)
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Signed(_name)
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Timer(_name) { 0 },
#define MEMFAULT_METRICS_KEY_DEFINE(_name, _type) \
  MEMFAULT_METRICS_STATE_HELPER_##_type(_name)
static sMemfaultMetricValueMetadata s_memfault_heartbeat_timer_values_metadata[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  // allocate at least one entry so we don't have an empty array in the situation
  // where no Timer metrics are defined:
  MEMFAULT_METRICS_KEY_DEFINE(_, kMemfaultMetricType_Timer)
  #undef MEMFAULT_METRICS_KEY_DEFINE
};
// Work-around for unused-macros error in case not all types are used in the .def file:
MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Unsigned(_)
MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Signed(_)

// We need a key-index table of pointers to timer metadata for fast lookups.
// The enum eMfltMetricsTimerIndex will create a subset of indexes for use
// in the s_memfault_heartbeat_timer_values_metadata[] table. The
// s_metric_timer_metadata_mapping[] table provides the mapping from the
// exhaustive list of keys to valid timer indexes or -1 if not a timer.
#define MEMFAULT_METRICS_KEY_DEFINE(_name, _type) \
   MEMFAULT_METRICS_STATE_HELPER_##_type(_name)

// String metrics are not present in eMfltMetricsTimerIndex
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length)

#undef MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Timer
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Timer(key_name) \
  kMfltMetricsTimerIndex_##key_name,

typedef enum MfltTimerIndex {
   #include "memfault/metrics/heartbeat_config.def"
   #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
} eMfltMetricsTimerIndex;

#undef MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Unsigned
#undef MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Signed
#undef MEMFAULT_METRICS_STRING_KEY_DEFINE

#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Unsigned(_name) \
  -1,
#define MEMFAULT_METRICS_STATE_HELPER_kMemfaultMetricType_Signed(_name) \
  -1,
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  -1,

static const int s_metric_timer_metadata_mapping[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};

static struct {
  const sMemfaultEventStorageImpl *storage_impl;
} s_memfault_metrics_ctx;

//
// Routines which can be overridden by customers
//

MEMFAULT_WEAK
void memfault_metrics_heartbeat_collect_data(void) { }


// Returns NULL if not a timer type or out of bounds index.
static sMemfaultMetricValueMetadata *prv_find_timer_metadatap(eMfltMetricsIndex metric_index) {
  if (metric_index >= MEMFAULT_ARRAY_SIZE(s_metric_timer_metadata_mapping)) {
    MEMFAULT_LOG_ERROR("Metric index %u exceeds expected array bounds %d\n",
                       metric_index, (int)MEMFAULT_ARRAY_SIZE(s_metric_timer_metadata_mapping));
    return NULL;
  }

  const int timer_index = s_metric_timer_metadata_mapping[metric_index];
  if (timer_index == -1) {
    return NULL;
  }

  return &s_memfault_heartbeat_timer_values_metadata[timer_index];
}

static eMemfaultMetricType prv_find_value_for_key(MemfaultMetricId key,
                                                  sMemfaultMetricValueInfo *value_info_out) {
  const size_t idx = (size_t)key._impl;
  if (idx >= MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys)) {
    *value_info_out = (sMemfaultMetricValueInfo){0};
    return kMemfaultMetricType_NumTypes;
  }

  // get the index for the value matching this key.
  eMfltMetricKeyToValueIndex key_index = s_memfault_heartbeat_key_to_valueindex[idx];
  // for scalar types, this will be the returned value pointer. non-scalars
  // will be handled in the switch below
  union MemfaultMetricValue *value_ptr = &s_memfault_heartbeat_values[key_index];

  eMemfaultMetricType key_type = s_memfault_heartbeat_keys[idx].type;
  switch (key_type) {
    case kMemfaultMetricType_String:
    {
      // get the string value associated with this key
      eMfltMetricStringKeyToIndex string_key_index = s_memfault_heartbeat_string_key_to_index[idx];
      // cast to uintptr_t then the final pointer type we want to drop the
      // 'const' and prevent tripping -Wcast-qual. this is safe, because we
      // never modify *value_ptr, only value_ptr->ptr, for non-scalar types.
      value_ptr = (union MemfaultMetricValue
                     *)(uintptr_t)&s_memfault_heartbeat_string_values[string_key_index];

    } break;

    case kMemfaultMetricType_Timer:
    case kMemfaultMetricType_Signed:
    case kMemfaultMetricType_Unsigned:
    case kMemfaultMetricType_NumTypes:  // To silence -Wswitch-enum
    default:
      break;
  }

  *value_info_out = (sMemfaultMetricValueInfo){
    .valuep = value_ptr,
    .meta_datap = prv_find_timer_metadatap((eMfltMetricsIndex)idx),
  };

  return key_type;
}

typedef bool (*MemfaultMetricKvIteratorCb)(void *ctx,
                                           const sMemfaultMetricKVPair *kv_pair,
                                           const sMemfaultMetricValueInfo *value_info);
static void prv_metric_iterator(void *ctx, MemfaultMetricKvIteratorCb cb) {
  for (uint32_t idx = 0; idx < MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys); ++idx) {
    const sMemfaultMetricKVPair *const kv_pair = &s_memfault_heartbeat_keys[idx];
    sMemfaultMetricValueInfo value_info = {0};

    (void)prv_find_value_for_key(kv_pair->key, &value_info);

    bool do_continue = cb(ctx, kv_pair, &value_info);

    if (!do_continue) {
      break;
    }
  }
}

static const sMemfaultMetricKVPair * prv_find_kvpair_for_key(MemfaultMetricId key) {
  const size_t idx = (size_t)key._impl;
  if (idx >= MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys)) {
    return NULL;
  }

  return &s_memfault_heartbeat_keys[idx];
}

static int prv_find_value_info_for_type(MemfaultMetricId key, eMemfaultMetricType expected_type,
                                        sMemfaultMetricValueInfo *value_info) {
  const eMemfaultMetricType type = prv_find_value_for_key(key, value_info);
  if (value_info->valuep == NULL) {
    return MEMFAULT_METRICS_KEY_NOT_FOUND;
  }
  if (type != expected_type) {
    // To easily get name of metric in gdb, p/s (eMfltMetricsIndex)0
    MEMFAULT_LOG_ERROR("Invalid type (%u vs %u) for key: %d", expected_type, type, key._impl);
    return MEMFAULT_METRICS_TYPE_INCOMPATIBLE;
  }
  return 0;
}

static int prv_find_and_set_value_for_key(
    MemfaultMetricId key, eMemfaultMetricType expected_type, union MemfaultMetricValue *new_value) {
  sMemfaultMetricValueInfo value_info = {0};
  int rv = prv_find_value_info_for_type(key, expected_type, &value_info);
  if (rv != 0) {
    return rv;
  }

  *value_info.valuep = *new_value;
  return 0;
}

int memfault_metrics_heartbeat_set_signed(MemfaultMetricId key, int32_t signed_value) {
  int rv;
  memfault_lock();
  {
    rv = prv_find_and_set_value_for_key(key, kMemfaultMetricType_Signed,
                                        &(union MemfaultMetricValue){.i32 = signed_value});
  }
  memfault_unlock();
  return rv;
}

int memfault_metrics_heartbeat_set_unsigned(MemfaultMetricId key, uint32_t unsigned_value) {
  int rv;
  memfault_lock();
  {
    rv = prv_find_and_set_value_for_key(key, kMemfaultMetricType_Unsigned,
                                        &(union MemfaultMetricValue){.u32 = unsigned_value});
  }
  memfault_unlock();
  return rv;
}

int memfault_metrics_heartbeat_set_string(MemfaultMetricId key, const char *value) {
  int rv;
  memfault_lock();
  {
    sMemfaultMetricValueInfo value_info = {0};
    rv = prv_find_value_info_for_type(key, kMemfaultMetricType_String, &value_info);
    const sMemfaultMetricKVPair *kv = prv_find_kvpair_for_key(key);

    // error if either the key is bad, or we can't find the kvpair for the key
    // (both checks should have the same result though)
    rv = (rv != 0 || kv == NULL) ? MEMFAULT_METRICS_KEY_NOT_FOUND : 0;

    if (rv == 0) {
      const size_t len = MEMFAULT_MIN(strlen(value), kv->range);
      memcpy(value_info.valuep->ptr, value, len);
      // null terminate
      ((char *)value_info.valuep->ptr)[len] = '\0';
    }
  }
  memfault_unlock();
  return rv;
}

typedef enum {
  kMemfaultTimerOp_Start,
  kMemfaultTimerOp_Stop,
  kMemfaultTimerOp_ForceValueUpdate,
} eMemfaultTimerOp;

static bool prv_update_timer_metric(const sMemfaultMetricValueInfo *value_info,
                                    eMemfaultTimerOp op) {
  sMemfaultMetricValueMetadata *meta_datap = value_info->meta_datap;
  const bool timer_running = meta_datap->is_running;

  // The timer is not running _and_ we received a Start request
  if (!timer_running && op == kMemfaultTimerOp_Start) {
    meta_datap->start_time_ms = memfault_platform_get_time_since_boot_ms();
    meta_datap->is_running = true;
    return true;
  }

  // the timer is running and we received a Stop or ForceValueUpdate request
  if (timer_running && op != kMemfaultTimerOp_Start) {
    const uint32_t stop_time_ms =
        memfault_platform_get_time_since_boot_ms() & ~MEMFAULT_METRICS_TIMER_VAL_MAX;
    const uint32_t start_time_ms = meta_datap->start_time_ms;

    uint32_t delta;
    if (stop_time_ms >= start_time_ms) {
      delta = stop_time_ms - start_time_ms;
    } else { // account for rollover
      delta = MEMFAULT_METRICS_TIMER_VAL_MAX - start_time_ms + stop_time_ms;
    }
    value_info->valuep->u32 += delta;

    if (op == kMemfaultTimerOp_Stop) {
      meta_datap->start_time_ms = 0;
      meta_datap->is_running = false;
    } else {
      meta_datap->start_time_ms = stop_time_ms;
    }

    return true;
  }

  // We were already in the state requested and no update took place
  return false;
}

static int prv_find_timer_metric_and_update(MemfaultMetricId key, eMemfaultTimerOp op) {
  sMemfaultMetricValueInfo value_info = {0};
  int rv = prv_find_value_info_for_type(key, kMemfaultMetricType_Timer, &value_info);
  if (rv != 0) {
    return rv;
  }

  // If the value did not change because the timer was already in the state requested return an
  // error code. This will make it easier for users of the external API to catch if their calls
  // were unbalanced.
  const bool did_update = prv_update_timer_metric(&value_info, op);
  return did_update ? 0 : MEMFAULT_METRICS_TYPE_NO_CHANGE;
}

int memfault_metrics_heartbeat_timer_start(MemfaultMetricId key) {
  int rv;
  memfault_lock();
  {
    rv = prv_find_timer_metric_and_update(key, kMemfaultTimerOp_Start);
  }
  memfault_unlock();
  return rv;
}

int memfault_metrics_heartbeat_timer_stop(MemfaultMetricId key) {
  int rv;
  memfault_lock();
  {
    rv = prv_find_timer_metric_and_update(key, kMemfaultTimerOp_Stop);
  }
  memfault_unlock();
  return rv;
}

static bool prv_tally_and_update_timer_cb(MEMFAULT_UNUSED void *ctx,
                                          const sMemfaultMetricKVPair *key,
                                          const sMemfaultMetricValueInfo *value) {
  if (key->type != kMemfaultMetricType_Timer) {
    return true;
  }

  prv_update_timer_metric(value, kMemfaultTimerOp_ForceValueUpdate);
  return true;
}

static void prv_reset_metrics(void) {
  // reset all scalar metric values
  memset(s_memfault_heartbeat_values, 0, sizeof(s_memfault_heartbeat_values));

  // reset all string metric values. -1 to skip the last, stub entry in the
  // table
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_string_values); i++) {
    // set null terminator
    if (s_memfault_heartbeat_string_values[i].ptr) {
      ((char *)s_memfault_heartbeat_string_values[i].ptr)[0] = 0;
    }
  }
}

static void prv_heartbeat_timer_update(void) {
  // force an update of the timer value for any actively running timers
  prv_metric_iterator(NULL, prv_tally_and_update_timer_cb);
}

//! Trigger an update of heartbeat metrics, serialize out to storage, and reset.
static void prv_heartbeat_timer(void) {
  prv_heartbeat_timer_update();
  memfault_metrics_heartbeat_collect_data();

  memfault_metrics_heartbeat_serialize(s_memfault_metrics_ctx.storage_impl);

  prv_reset_metrics();
}

static int prv_find_key_and_add(MemfaultMetricId key, int32_t amount) {
  sMemfaultMetricValueInfo value_info = {0};
  const eMemfaultMetricType type = prv_find_value_for_key(key, &value_info);
  if (value_info.valuep == NULL) {
    return MEMFAULT_METRICS_KEY_NOT_FOUND;
  }
  union MemfaultMetricValue *value = value_info.valuep;

  switch ((int)type) {
    case kMemfaultMetricType_Signed: {
      // Clip in case of overflow:
      int64_t new_value = (int64_t)value->i32 + (int64_t)amount;
      if (new_value > INT32_MAX) {
        value->i32 = INT32_MAX;
      } else if (new_value < INT32_MIN) {
        value->i32 = INT32_MIN;
      } else {
        value->i32 = new_value;
      }
      break;
    }

    case kMemfaultMetricType_Unsigned: {
      uint32_t new_value = value->u32 + (uint32_t)amount;
      const bool amount_is_positive = amount > 0;
      const bool did_increase = new_value > value->u32;
      // Clip in case of overflow:
      if ((uint32_t)amount_is_positive ^ (uint32_t)did_increase) {
        new_value = amount_is_positive ? UINT32_MAX : 0;
      }
      value->u32 = new_value;
      break;
    }

    case kMemfaultMetricType_Timer:
    case kMemfaultMetricType_String:
    case kMemfaultMetricType_NumTypes: // To silence -Wswitch-enum
    default:
      // To easily get name of metric in gdb, p/s (eMfltMetricsIndex)0
      MEMFAULT_LOG_ERROR("Can only add to number types (key: %d)", key._impl);
      return MEMFAULT_METRICS_TYPE_INCOMPATIBLE;
  }
  return 0;
}

int memfault_metrics_heartbeat_add(MemfaultMetricId key, int32_t amount) {
  int rv;
  memfault_lock();
  {
    rv = prv_find_key_and_add(key, amount);
  }
  memfault_unlock();
  return rv;
}

static int prv_find_key_of_type(MemfaultMetricId key, eMemfaultMetricType expected_type,
                                union MemfaultMetricValue **value_out) {
  sMemfaultMetricValueInfo value_info = {0};
  const eMemfaultMetricType type = prv_find_value_for_key(key, &value_info);
  if (value_info.valuep == NULL) {
    return MEMFAULT_METRICS_KEY_NOT_FOUND;
  }
  if (type != expected_type) {
    return MEMFAULT_METRICS_TYPE_INCOMPATIBLE;
  }
  *value_out = value_info.valuep;
  return 0;
}

int memfault_metrics_heartbeat_read_unsigned(MemfaultMetricId key, uint32_t *read_val) {
  if (read_val == NULL) {
    return MEMFAULT_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  memfault_lock();
  {
    union MemfaultMetricValue *value;
    rv = prv_find_key_of_type(key, kMemfaultMetricType_Unsigned, &value);
    if (rv == 0) {
      *read_val = value->u32;
    }
  }
  memfault_unlock();
  return rv;
}

int memfault_metrics_heartbeat_read_signed(MemfaultMetricId key, int32_t *read_val) {
  if (read_val == NULL) {
    return MEMFAULT_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  memfault_lock();
  {
    union MemfaultMetricValue *value;
    rv = prv_find_key_of_type(key, kMemfaultMetricType_Signed, &value);
    if (rv == 0) {
      *read_val = value->i32;
    }
  }
  memfault_unlock();
  return rv;
}

int memfault_metrics_heartbeat_timer_read(MemfaultMetricId key, uint32_t *read_val) {
  if (read_val == NULL) {
    return MEMFAULT_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  memfault_lock();
  {
    union MemfaultMetricValue *value;
    rv = prv_find_key_of_type(key, kMemfaultMetricType_Timer, &value);
    if (rv == 0) {
      *read_val = value->u32;
    }
  }
  memfault_unlock();
  return rv;
}
int memfault_metrics_heartbeat_read_string(MemfaultMetricId key, char *read_val,
                                           size_t read_val_len) {
  if ((read_val == NULL) || (read_val_len == 0)) {
    return MEMFAULT_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  memfault_lock();
  {
    union MemfaultMetricValue *value;
    rv = prv_find_key_of_type(key, kMemfaultMetricType_String, &value);
    const sMemfaultMetricKVPair *kv = prv_find_kvpair_for_key(key);

    rv = (rv != 0 || kv == NULL) ? MEMFAULT_METRICS_KEY_NOT_FOUND : 0;

    if (rv == 0) {
      // copy up to the min of the length of the string and the length of the
      // provided buffer
      size_t len = strlen(value->ptr) + 1;
      memcpy(read_val, value->ptr, MEMFAULT_MIN(len, read_val_len));
      // always null terminate
      read_val[read_val_len - 1] = '\0';
    }
  }
  memfault_unlock();

  return rv;
}

typedef struct {
  MemfaultMetricIteratorCallback user_cb;
  void *user_ctx;
} sMetricHeartbeatIterateCtx;

static bool prv_metrics_heartbeat_iterate_cb(void *ctx,
                                             const sMemfaultMetricKVPair *key_info,
                                             const sMemfaultMetricValueInfo *value_info) {
  sMetricHeartbeatIterateCtx *ctx_info = (sMetricHeartbeatIterateCtx *)ctx;

  sMemfaultMetricInfo info = {
    .key = key_info->key,
    .type = key_info->type,
    .val = *value_info->valuep
  };
  return ctx_info->user_cb(ctx_info->user_ctx, &info);
}

void memfault_metrics_heartbeat_iterate(MemfaultMetricIteratorCallback cb, void *ctx) {
  memfault_lock();
  {
    sMetricHeartbeatIterateCtx user_ctx = {
      .user_cb = cb,
      .user_ctx = ctx,
    };
    prv_metric_iterator(&user_ctx, prv_metrics_heartbeat_iterate_cb);
  }
  memfault_unlock();
}

size_t memfault_metrics_heartbeat_get_num_metrics(void) {
  return MEMFAULT_ARRAY_SIZE(s_memfault_heartbeat_keys);
}

#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) \
  MEMFAULT_QUOTE(key_name),
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  MEMFAULT_QUOTE(key_name),

static const char *s_idx_to_metric_name[] = {
  #include "memfault/metrics/heartbeat_config.def"
  #include MEMFAULT_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef MEMFAULT_METRICS_KEY_DEFINE
  #undef MEMFAULT_METRICS_STRING_KEY_DEFINE
};

static bool prv_heartbeat_debug_print(MEMFAULT_UNUSED void *ctx,
                                      const sMemfaultMetricInfo *metric_info) {
  const MemfaultMetricId *key = &metric_info->key;
  const union MemfaultMetricValue *value = &metric_info->val;

  const char *key_name = s_idx_to_metric_name[key->_impl];

  switch (metric_info->type) {
    case kMemfaultMetricType_Unsigned:
    case kMemfaultMetricType_Timer:
      MEMFAULT_LOG_DEBUG("  %s: %" PRIu32, key_name, value->u32);
      break;
    case kMemfaultMetricType_Signed:
      MEMFAULT_LOG_DEBUG("  %s: %" PRIi32, key_name, value->i32);
      break;
    case kMemfaultMetricType_String:
      MEMFAULT_LOG_DEBUG("  %s: \"%s\"", key_name, (const char *)value->ptr);
      break;

    case kMemfaultMetricType_NumTypes: // To silence -Wswitch-enum
    default:
      MEMFAULT_LOG_DEBUG("  %s: <unknown type>", key_name);
      break;
  }

  return true; // continue iterating
}

void memfault_metrics_heartbeat_debug_print(void) {
  prv_heartbeat_timer_update();
  MEMFAULT_LOG_DEBUG("Heartbeat keys/values:");
  memfault_metrics_heartbeat_iterate(prv_heartbeat_debug_print, NULL);
}

void memfault_metrics_heartbeat_debug_trigger(void) {
  prv_heartbeat_timer();
}

int memfault_metrics_boot(const sMemfaultEventStorageImpl *storage_impl,
                          const sMemfaultMetricBootInfo *info) {
  if (storage_impl == NULL || info == NULL) {
    return MEMFAULT_METRICS_TYPE_BAD_PARAM;
  }

  s_memfault_metrics_ctx.storage_impl = storage_impl;
  prv_reset_metrics();

  const bool success = memfault_platform_metrics_timer_boot(
      MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS, prv_heartbeat_timer);
  if (!success) {
    return MEMFAULT_METRICS_TIMER_BOOT_FAILED;
  }

  if (!memfault_serializer_helper_check_storage_size(
      storage_impl, memfault_metrics_heartbeat_compute_worst_case_storage_size, "metrics")) {
    return MEMFAULT_METRICS_STORAGE_TOO_SMALL;
  }

  int rv = memfault_metrics_heartbeat_timer_start(
      MEMFAULT_METRICS_KEY(MemfaultSdkMetric_IntervalMs));
  if (rv != 0) {
    return rv;
  }

  rv = memfault_metrics_heartbeat_set_unsigned(
      MEMFAULT_METRICS_KEY(MemfaultSdkMetric_UnexpectedRebootCount), info->unexpected_reboot_count);
  if (rv != 0) {
    return rv;
  }

  return 0;
}
