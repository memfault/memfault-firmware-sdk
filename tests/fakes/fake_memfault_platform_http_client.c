//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of the http client platform API

#include "memfault/http/platform/http_client.h"

#include "memfault/core/compiler.h"

int memfault_platform_http_response_get_status(
    MEMFAULT_UNUSED const sMfltHttpResponse *response, uint32_t *status_out) {
  if (status_out) {
    *status_out = 200;
  }
  return 0;
}

static sMfltHttpClient *s_client = (sMfltHttpClient *)~0;

sMfltHttpClient *memfault_platform_http_client_create(void) {
  return s_client;
}

int memfault_platform_http_client_post_data(
    MEMFAULT_UNUSED sMfltHttpClient *client,
    MEMFAULT_UNUSED MemfaultHttpClientResponseCallback callback,
    MEMFAULT_UNUSED void *ctx) {
  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
    MEMFAULT_UNUSED sMfltHttpClient *client,
    MEMFAULT_UNUSED uint32_t timeout_ms) {
  return 0;
}

int memfault_platform_http_client_destroy(MEMFAULT_UNUSED sMfltHttpClient *client) {
  return 0;
}
