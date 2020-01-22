//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Reads the current reboot tracking information and converts it into an "trace" event which can
//! be sent to the Memfault Cloud

#include "memfault_reboot_tracking_private.h"

#include <string.h>

#include "memfault/core/data_packetizer_source.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/event_storage.h"
#include "memfault/core/event_storage_implementation.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/core/serializer_helper.h"
#include "memfault/core/serializer_key_ids.h"
#include "memfault/util/cbor.h"

#define MEMFAULT_REBOOT_TRACKING_BAD_PARAM (-1)
#define MEMFAULT_REBOOT_TRACKING_STORAGE_TOO_SMALL (-2)

static bool prv_encode_event_key_uint32_pair(
    sMemfaultCborEncoder *encoder, eMemfaultEventKey key, uint32_t value) {
  return memfault_cbor_encode_unsigned_integer(encoder, key) &&
      memfault_cbor_encode_unsigned_integer(encoder, value);
}

static bool prv_serialize_reboot_info(sMemfaultCborEncoder *e,
                                      const sMfltResetReasonInfo *info) {
  const size_t top_level_num_pairs = 1 /* type */ + 4 /* device info */ +
      1 /* cbor schema version */ + 1 /* event_info */;
  memfault_cbor_encode_dictionary_begin(e, top_level_num_pairs);

  if (!prv_encode_event_key_uint32_pair(e, kMemfaultEventKey_Type, kMemfaultEventType_Trace)) {
    return false;
  }

  memfault_serializer_helper_encode_version_info(e);

  const size_t num_entries = 1 /* reason */ + 1 /* coredump_saved */ +
      ((info->pc != 0) ? 1 : 0) +
      ((info->lr != 0) ? 1 : 0) +
      ((info->reset_reason_reg0 != 0) ? 1 : 0);

  if (!memfault_cbor_encode_unsigned_integer(e, kMemfaultEventKey_EventInfo) ||
      !memfault_cbor_encode_dictionary_begin(e, num_entries)) {
    return false;
  }

  if (!memfault_serializer_helper_encode_uint32_kv_pair(
          e, kMemfaultTraceInfoEventKey_Reason, info->reason)) {
    return false;
  }

  bool success = true;
  if (info->pc) {
    success = memfault_serializer_helper_encode_uint32_kv_pair(e,
        kMemfaultTraceInfoEventKey_ProgramCounter, info->pc);
  }

  if (success && info->lr) {
    success = memfault_serializer_helper_encode_uint32_kv_pair(e,
        kMemfaultTraceInfoEventKey_LinkRegister, info->lr);
  }

  if (success && info->reset_reason_reg0) {
    success = memfault_serializer_helper_encode_uint32_kv_pair(e,
        kMemfaultTraceInfoEventKey_McuReasonRegister, info->reset_reason_reg0);
  }

  if (success) {
    success = memfault_serializer_helper_encode_uint32_kv_pair(
        e, kMemfaultTraceInfoEventKey_CoredumpSaved, info->coredump_saved);
  }

  return success;
}

typedef struct {
  const sMemfaultEventStorageImpl *storage_impl;
} sMemfaultResetReasonEncoderCtx;

static void prv_encoder_write_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  sMemfaultResetReasonEncoderCtx *encoder = (sMemfaultResetReasonEncoderCtx *)ctx;
  encoder->storage_impl->append_data_cb(buf, buf_len);
}

size_t memfault_reboot_tracking_compute_worst_case_storage_size(void) {
  // a reset reason with maximal values so we can compute the worst case encoding size
  sMfltResetReasonInfo reset_reason = {
    .reason = kMfltRebootReason_HardFault,
    .pc = UINT32_MAX,
    .lr = UINT32_MAX,
    .reset_reason_reg0 = UINT32_MAX,
    .coredump_saved = 1,
  };

  sMemfaultCborEncoder encoder = { 0 };
  memfault_cbor_encoder_size_only_init(&encoder);
  prv_serialize_reboot_info(&encoder, &reset_reason);
  return memfault_cbor_encoder_deinit(&encoder);
}

int  memfault_reboot_tracking_collect_reset_info(const sMemfaultEventStorageImpl *impl) {
  if (impl == NULL) {
    return MEMFAULT_REBOOT_TRACKING_BAD_PARAM;
  }

  const size_t worst_case_size_needed =
      memfault_reboot_tracking_compute_worst_case_storage_size();
  const size_t storage_max_size = impl->get_storage_size_cb();
  if (worst_case_size_needed > storage_max_size) {
    MEMFAULT_LOG_WARN("Event storage (%d) smaller than largest reset reason (%d)",
                      (int)storage_max_size, (int)worst_case_size_needed);
    // we'll fall through and try to encode anyway and later return a failure
    // code if the event could not be stored. This line is here to give the user
    // an idea of how they should size things
  }

  sMfltResetReasonInfo info;
  if (!memfault_reboot_tracking_read_reset_info(&info)) {
    return 0;
  }

  size_t space_available = impl->begin_write_cb();
  bool success;
  {
    sMemfaultCborEncoder encoder = { 0 };
    sMemfaultResetReasonEncoderCtx ctx = {
      .storage_impl = impl,
    };
    memfault_cbor_encoder_init(&encoder, prv_encoder_write_cb, &ctx, space_available);
    success = prv_serialize_reboot_info(&encoder, &info);
    memfault_cbor_encoder_deinit(&encoder);
  }
  const bool rollback = !success;
  impl->finish_write_cb(rollback);

  if (!success) {
    MEMFAULT_LOG_WARN("Event storage (%d) smaller than largest reset reason (%d)",
                      (int)storage_max_size, (int)worst_case_size_needed);
    return MEMFAULT_REBOOT_TRACKING_STORAGE_TOO_SMALL;
  }

  memfault_reboot_tracking_clear_reset_info();
  return 0;
}
