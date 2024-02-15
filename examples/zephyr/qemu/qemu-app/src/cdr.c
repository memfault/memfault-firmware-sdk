//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Sample CDR implementation. See https://mflt.io/custom-data-recordings for
//! more information

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

#include "memfault/components.h"
// clang-format on

LOG_MODULE_REGISTER(cdr);

#include "memfault/components.h"

// Only read the CDR payload when this is set
static bool s_cdr_ready_to_send = false;
// keep track of the offset into the CDR data; the packetizer may call
// repeatedly depending on chunk size constraints
static size_t s_cdr_read_offset = 0;

// set of MIME types for this payload
static const char *mimetypes[] = { MEMFAULT_CDR_TEXT };

// the actual payload is just a simple string in this example
static const char cdr_payload[] = "hello cdr!";

// the CDR metadata structure
static sMemfaultCdrMetadata s_cdr_metadata = {
  .start_time =
    {
      .type = kMemfaultCurrentTimeType_Unknown,
    },
  .mimetypes = mimetypes,
  .num_mimetypes = MEMFAULT_ARRAY_SIZE(mimetypes),

  // in this case, the data size is fixed. typically it would be set in the
  // prv_has_cdr_cb() function, and likely variable size
  .data_size_bytes = sizeof(cdr_payload) - 1,
  .duration_ms = 0,

  .collection_reason = "example cdr upload",
};

// called to see if there's any data available; uses the *metadata output to set
// the header fields in the chunked message sent to Memfault
static bool prv_has_cdr_cb(sMemfaultCdrMetadata *metadata) {
  *metadata = s_cdr_metadata;
  return s_cdr_ready_to_send;
}

// called by the packetizer to read up to .data_size_bytes of CDR data
static bool prv_read_data_cb(uint32_t offset, void *data, size_t data_len) {
  if (offset != s_cdr_read_offset) {
    LOG_ERR("Unexpected offset: %d vs %d", offset, s_cdr_read_offset);
    s_cdr_read_offset = 0;
    return false;
  }

  const size_t copy_len = MEMFAULT_MIN(data_len, sizeof(cdr_payload) - offset);
  LOG_INF("Reading %d bytes from offset %d", copy_len, offset);

  memcpy(data, ((uint8_t *)cdr_payload) + offset, copy_len);
  s_cdr_read_offset += copy_len;
  return true;
}

// called when all data has been drained from the read callback
static void prv_mark_cdr_read_cb(void) {
  s_cdr_ready_to_send = false;
  // only reset this offset when the data has been read
  s_cdr_read_offset = 0;
}

// Set up the callback functions. This CDR Source Implementation structure must
// have a lifetime through the duration of the program- typically setting it to
// 'const' is appropriate
const sMemfaultCdrSourceImpl g_custom_data_recording_source = {
  .has_cdr_cb = prv_has_cdr_cb,
  .read_data_cb = prv_read_data_cb,
  .mark_cdr_read_cb = prv_mark_cdr_read_cb,
};

//! Shell function to stage a sample CDR payload for collection next time
//! the packetizer runs
static int prv_stage_cdr(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(shell);
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  s_cdr_ready_to_send = true;
  s_cdr_read_offset = 0;

  sMemfaultCurrentTime timestamp;
  if (memfault_platform_time_get_current(&timestamp)) {
    s_cdr_metadata.start_time = timestamp;
  } else {
    s_cdr_metadata.start_time.type = kMemfaultCurrentTimeType_Unknown;
  }

  return 0;
}

SHELL_CMD_REGISTER(cdr, NULL, "Stages a sample CDR payload", prv_stage_cdr);
