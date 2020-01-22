//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Utility responsible for serializing out a Memfault Heartbeat. Today, the serialization format
//! used is cbor but the actual format used is opaque to an end user and no assumptions should be
//! made in user code based on it.

#include "memfault/metrics/serializer.h"

#include <stdio.h>
#include <stddef.h>

#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/serializer_helper.h"
#include "memfault/core/serializer_key_ids.h"
#include "memfault/metrics/utils.h"
#include "memfault/util/cbor.h"

typedef struct {
  sMemfaultCborEncoder encoder;
  bool compute_worst_case_size;
  bool encode_success;
} sMemfaultSerializerState;

static bool prv_metric_heartbeat_writer(void *ctx, const sMemfaultMetricInfo *metric_info) {
  sMemfaultSerializerState *state = (sMemfaultSerializerState *)ctx;
  sMemfaultCborEncoder *encoder = &state->encoder;

  // encode the key
  state->encode_success = memfault_cbor_encode_string(encoder, metric_info->key._impl);
  if (!state->encode_success) {
    return false;
  }

  // encode the value
  switch (metric_info->type) {
    case kMemfaultMetricType_Timer:
    case kMemfaultMetricType_Unsigned: {
      const uint32_t value = state->compute_worst_case_size ? UINT32_MAX : metric_info->val.u32;
      state->encode_success = memfault_cbor_encode_unsigned_integer(encoder, value);
      break;
    }
    case kMemfaultMetricType_Signed: {
      const int32_t value = state->compute_worst_case_size ? INT32_MIN : metric_info->val.i32;
      state->encode_success = memfault_cbor_encode_signed_integer(encoder, value);
      break;
    }
    default:
      break;
  }

  // only continue iterating if the encode was successful
  return state->encode_success;
}

static bool prv_serialize_latest_heartbeat_and_deinit(sMemfaultSerializerState *state, size_t *total_bytes) {
  bool success = false;

  sMemfaultCborEncoder *encoder = &state->encoder;
  const size_t top_level_num_pairs = 1 /* type */ + 4 /* device info */ + 1 /* cbor schema version */ +
      1 /* event_info */;
  memfault_cbor_encode_dictionary_begin(encoder, top_level_num_pairs);

  if (!memfault_serializer_helper_encode_uint32_kv_pair(
          encoder, kMemfaultEventKey_Type, kMemfaultEventType_Heartbeat)) {
    return false;
  }

  if (!memfault_serializer_helper_encode_version_info(encoder)) {
    goto cleanup;
  }

  // Encode up to "metrics:" section
  const size_t num_metrics = memfault_metrics_heartbeat_get_num_metrics();
  if (!memfault_cbor_encode_unsigned_integer(encoder, kMemfaultEventKey_EventInfo) ||
      !memfault_cbor_encode_dictionary_begin(encoder, 1) ||
      !memfault_cbor_encode_unsigned_integer(encoder, kMemfaultHeartbeatInfoKey_Metrics) ||
      !memfault_cbor_encode_dictionary_begin(encoder, num_metrics)) {
    goto cleanup;
  }


  memfault_metrics_heartbeat_iterate(prv_metric_heartbeat_writer, state);
  success = state->encode_success;

cleanup:
  *total_bytes = memfault_cbor_encoder_deinit(encoder);
  return success;
}

size_t memfault_metrics_heartbeat_compute_worst_case_storage_size(void) {
  sMemfaultSerializerState state = { .compute_worst_case_size = true };
  memfault_cbor_encoder_size_only_init(&state.encoder);
  size_t bytes_encoded = 0;
  prv_serialize_latest_heartbeat_and_deinit(&state, &bytes_encoded);
  return bytes_encoded;
}

typedef struct {
  const sMemfaultEventStorageImpl *storage_impl;
} sMemfaultMetricEncoderCtx;

static void prv_encoder_write_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  sMemfaultMetricEncoderCtx *encoder = (sMemfaultMetricEncoderCtx *)ctx;
  encoder->storage_impl->append_data_cb(buf, buf_len);
}

bool memfault_metrics_heartbeat_serialize(const sMemfaultEventStorageImpl *storage_impl) {
  // Build a heartbeat event, which looks like this:
  // {
  //    "type": "heartbeat",
  //    "device_serial": "DAABBCCDD",
  //    "software_type": "main",
  //    "software_version": "1.2.3",
  //    "hardware_version": "evt_24",
  //    "event_info": {
  //         "metrics": {
  //          ... heartbeat metrics ...
  //    }
  // }
  // NOTE: "sdk_version" is not included, but derived from the CborSchemaVersion

  // NOTE: We'll always attempt to serialize the heartbeat and rollback if we are out of space
  // avoiding the need to serialize the data twice
  size_t space_available = storage_impl->begin_write_cb();
  size_t bytes_encoded = 0;
  bool success;
  {
    sMemfaultMetricEncoderCtx ctx = {
      .storage_impl = storage_impl,
    };
    sMemfaultSerializerState state = { 0 };
    memfault_cbor_encoder_init(&state.encoder, prv_encoder_write_cb, &ctx, space_available);
    success = prv_serialize_latest_heartbeat_and_deinit(&state, &bytes_encoded);
  }
  const bool rollback = !success;
  storage_impl->finish_write_cb(rollback);

  if (!success) {
    MEMFAULT_LOG_ERROR("%s storage out of space", __func__);
  }

  return success;
}
