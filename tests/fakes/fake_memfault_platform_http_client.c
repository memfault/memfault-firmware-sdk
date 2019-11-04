//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of the http client platform API

#include "memfault/http/platform/http_client.h"

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response, uint32_t *status_out) {
  if (status_out) {
    *status_out = 200;
  }
  return 0;
}

static sMfltHttpClient *s_client = (sMfltHttpClient *)~0;

sMfltHttpClient *memfault_platform_http_client_create(void) {
  return s_client;
}

int memfault_platform_http_client_post_data(sMfltHttpClient *client,
                                            MemfaultHttpClientResponseCallback callback, void *ctx) {
  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  return 0;
}

int memfault_platform_http_client_destroy(sMfltHttpClient *client) {
  return 0;
}
