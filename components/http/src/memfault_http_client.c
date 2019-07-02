//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Memfault HTTP Client implementation which can be used to send data to the Memfault cloud for
//! processing

#include "memfault/http/http_client.h"

#include "memfault/panics/coredump.h"
#include "memfault/core/debug_log.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/http/platform/http_client.h"

#include <stdio.h>

//! HTTP status that is returned when a coredump is uploaded that already exists.
#define MEMFAULT_HTTP_API_COREDUMP_HTTP_STATUS_ALREADY_EXISTS (409)

static const char *prv_get_scheme(void) {
  return g_mflt_http_client_config.api_no_tls ? "http" : "https";
}

MemfaultReturnCode memfault_http_build_url(char url_buffer[MEMFAULT_HTTP_URL_BUFFER_SIZE], const char *subpath) {
  const int rv = snprintf(url_buffer, MEMFAULT_HTTP_URL_BUFFER_SIZE, "%s://%s" MEMFAULT_HTTP_API_PREFIX "%s",
                          prv_get_scheme(), MEMFAULT_HTTP_GET_API_HOST(), subpath);
  if (MEMFAULT_HTTP_URL_BUFFER_SIZE <= rv) {
    return MemfaultReturnCode_Error;
  }
  return MemfaultReturnCode_Ok;
}

sMfltHttpClient *memfault_http_client_create(void) {
  return memfault_platform_http_client_create();
}

static void prv_handle_post_coredump_response(const sMfltHttpResponse *response, void *ctx) {
  if (!response) {
    return;  // Request failed
  }
  uint32_t http_status = 0;
  const MemfaultReturnCode rv = memfault_platform_http_response_get_status(response, &http_status);
  if (rv != MemfaultReturnCode_Ok) {
    MEMFAULT_LOG_ERROR("Request failed. No HTTP status: %d", rv);
    return;
  }
  if (http_status != MEMFAULT_HTTP_API_COREDUMP_HTTP_STATUS_ALREADY_EXISTS &&
      (http_status < 200 || http_status >= 300)) {
    // Redirections are expected to be handled by the platform implementation
    MEMFAULT_LOG_ERROR("Request failed. HTTP Status: %"PRIu32, http_status);
    return;
  }
  memfault_platform_coredump_storage_clear();
}

MemfaultReturnCode memfault_http_client_post_coredump(sMfltHttpClient *client) {
  if (!client) {
    return MemfaultReturnCode_InvalidInput;
  }
  return memfault_platform_http_client_post_coredump(client, prv_handle_post_coredump_response, NULL);
}

MemfaultReturnCode memfault_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  if (!client) {
    return MemfaultReturnCode_InvalidInput;
  }
  return memfault_platform_http_client_wait_until_requests_completed(client, timeout_ms);
}

MemfaultReturnCode memfault_http_client_destroy(sMfltHttpClient *client) {
  if (!client) {
    return MemfaultReturnCode_InvalidInput;
  }
  return memfault_platform_http_client_destroy(client);
}
