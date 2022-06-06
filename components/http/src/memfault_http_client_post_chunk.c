//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements conveninece API for posting a single chunk of Memfault data

#include "memfault/http/http_client.h"

#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/data_packetizer.h"

int memfault_http_client_post_chunk(void) {
  // A pre-flight check before we attempt to setup an HTTP client
  // If there's no data to send, just early return
  bool more_data = memfault_packetizer_data_available();
  if (!more_data) {
    // no new data to post
    return kMfltPostDataStatus_NoDataFound;
  }

  sMfltHttpClient *http_client = memfault_http_client_create();
  if (!http_client) {
    MEMFAULT_LOG_ERROR("Failed to create HTTP client");
    return MemfaultInternalReturnCode_Error;
  }

  const int rv = memfault_http_client_post_data(http_client);
  if ((eMfltPostDataStatus)rv !=  kMfltPostDataStatus_Success) {
    MEMFAULT_LOG_ERROR("Failed to post chunk: rv=%d", rv);
  }
  const uint32_t timeout_ms = 30 * 1000;
  memfault_http_client_wait_until_requests_completed(http_client, timeout_ms);
  memfault_http_client_destroy(http_client);
  return rv;
}
