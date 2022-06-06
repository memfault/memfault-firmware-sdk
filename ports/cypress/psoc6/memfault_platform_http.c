//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements dependency functions required to utilize Memfault's http client
//! for posting data collected via HTTPS

#include <stdbool.h>

#include "memfault/components.h"

#include "cy_secure_sockets.h"
#include "cy_tls.h"

struct MfltHttpClient {
  bool active;
  cy_socket_t handle;
  cy_socket_sockaddr_t sock_addr;
};

typedef struct MfltHttpResponse {
  uint32_t status_code;
} sMfltHttpResponse;

static sMfltHttpClient s_client;

static cy_rslt_t prv_teardown_connection(sMfltHttpClient *client) {
  cy_rslt_t result = cy_socket_disconnect(client->handle, 0);
  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR("cy_socket_disconnect failed: rv=0x%x", (int)result);
  }

  cy_socket_delete(client->handle);
  client->active = false;
  return 0;
}

static cy_rslt_t prv_disconnect_handler(cy_socket_t socket_handle, void *arg) {
  MEMFAULT_LOG_DEBUG("Memfault Socket Disconnected dropped");
  prv_teardown_connection(&s_client);
  return 0;
}

static cy_rslt_t prv_open_socket(sMfltHttpClient *client) {
  cy_rslt_t result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                                      CY_SOCKET_IPPROTO_TLS, &client->handle);

  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR("cy_socket_create failed, rv=0x%x", (int)result);
    return result;
  }

  cy_socket_opt_callback_t disconnect_listener = {
      .callback = prv_disconnect_handler, .arg = NULL};

  result = cy_socket_setsockopt(
      client->handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_DISCONNECT_CALLBACK,
      &disconnect_listener, sizeof(cy_socket_opt_callback_t));
  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR(
        "cy_socket_setsockopt CY_SOCKET_SO_DISCONNECT_CALLBACK failed, rv=0x%x", (int)result);
    return result;
  }

  cy_socket_tls_auth_mode_t tls_auth_mode = CY_SOCKET_TLS_VERIFY_REQUIRED;
  result = cy_socket_setsockopt(client->handle, CY_SOCKET_SOL_TLS,
                                CY_SOCKET_SO_TLS_AUTH_MODE, &tls_auth_mode,
                                sizeof(cy_socket_tls_auth_mode_t));
  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR("cy_socket_setsockopt CY_SOCKET_SO_TLS_AUTH_MODE failed, rv=0x%x",
                       (int)result);
  }

  result = cy_socket_connect(client->handle, &client->sock_addr,
                             sizeof(cy_socket_sockaddr_t));
  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR("cy_socket_connect failed, 0x%x", (int)result);
  }

  return result;
}

sMfltHttpClient *memfault_platform_http_client_create(void) {
  if (s_client.active) {
    MEMFAULT_LOG_ERROR("Memfault HTTP client already in use");
    return NULL;
  }

  s_client = (sMfltHttpClient) {
    .sock_addr = {
      .port = MEMFAULT_HTTP_GET_CHUNKS_API_PORT(),
    },
  };

  cy_rslt_t result = cy_socket_gethostbyname(
      MEMFAULT_HTTP_CHUNKS_API_HOST, CY_SOCKET_IP_VER_V4, &s_client.sock_addr.ip_address);
  if (result != CY_RSLT_SUCCESS) {
    MEMFAULT_LOG_ERROR("DNS lookup failed: 0x%x", (int)result);
    return NULL;
  }

  result = prv_open_socket(&s_client);
  if (result != CY_RSLT_SUCCESS) {
    return NULL;
  }

  s_client.active = true;
  return &s_client;
}


static bool prv_try_send(sMfltHttpClient *client, const uint8_t *buf, size_t buf_len) {
  cy_rslt_t idx = 0;
  while (idx != buf_len) {
    uint32_t bytes_sent = 0;
    int rv = cy_socket_send(client->handle, &buf[idx], buf_len - idx, CY_SOCKET_FLAGS_NONE, &bytes_sent);
    if (rv == CY_RSLT_SUCCESS && bytes_sent > 0) {
      idx += bytes_sent;
      continue;
    }

    MEMFAULT_LOG_ERROR("Data Send Error: bytes_sent=%d, cy_rslt=0x%x", (int)bytes_sent, rv);
    return false;

  }
  return true;
}

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  sMfltHttpClient *client = (sMfltHttpClient *)ctx;
  return prv_try_send(client, data, data_len);
}

static bool prv_read_socket_data(sMfltHttpClient *client, void *buf, size_t *buf_len) {
  uint32_t buf_len_out;
  cy_rslt_t result = cy_socket_recv(client->handle, buf, *buf_len, CY_SOCKET_FLAGS_NONE, &buf_len_out);
  *buf_len = (size_t)buf_len_out;
  return result == CY_RSLT_SUCCESS;
}

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response, uint32_t *status_out) {
  MEMFAULT_SDK_ASSERT(response != NULL);

  *status_out = response->status_code;
  return 0;
}

static int prv_wait_for_http_response(sMfltHttpClient *client) {
  sMemfaultHttpResponseContext ctx = { 0 };
  while (1) {
    // We don't expect any response that needs to be parsed so
    // just use an arbitrarily small receive buffer
    char buf[32];
    size_t bytes_read = sizeof(buf);
    if (!prv_read_socket_data(client, buf, &bytes_read)) {
      return -1;
    }

    bool done = memfault_http_parse_response(&ctx, buf, bytes_read);
    if (done) {
      MEMFAULT_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!",
                         (int)ctx.parse_error, ctx.http_status_code);
      MEMFAULT_LOG_DEBUG("Body: %s", ctx.http_body);
      return ctx.http_status_code;
    }
  }
}

int memfault_platform_http_client_post_data(
    sMfltHttpClient *client, MemfaultHttpClientResponseCallback callback, void *ctx) {
  if (!client->active) {
    return -1;
  }

  const sPacketizerConfig cfg = {
    // let a single msg span many "memfault_packetizer_get_next" calls
    .enable_multi_packet_chunk = true,
  };

  // will be populated with size of entire message queued for sending
  sPacketizerMetadata metadata;
  const bool data_available = memfault_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    return kMfltPostDataStatus_NoDataFound;
  }

  memfault_http_start_chunk_post(prv_send_data, client, metadata.single_chunk_message_length);

  // Drain all the data that is available to be sent
  while (1) {
    // Arbitrarily sized send buffer.
    uint8_t buf[128];
    size_t buf_len = sizeof(buf);
    eMemfaultPacketizerStatus status = memfault_packetizer_get_next(buf, &buf_len);
    if (status == kMemfaultPacketizerStatus_NoMoreData) {
      break;
    }

    if (!prv_try_send(client, buf, buf_len)) {
      // unexpected failure, abort in-flight transaction
      memfault_packetizer_abort();
      return false;
    }

    if (status == kMemfaultPacketizerStatus_EndOfChunk) {
      break;
    }
  }

  // we've sent a chunk, drain status
  sMfltHttpResponse response = {
    .status_code = prv_wait_for_http_response(client),
  };

  if (callback) {
    callback(&response, ctx);
  }

  return 0;
}

int memfault_platform_http_client_destroy(sMfltHttpClient *client) {
  if (!client->active) {
    return -1;
  }

  prv_teardown_connection(client);
  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  // No-op because memfault_platform_http_client_post_data() is synchronous
  return 0;
}
