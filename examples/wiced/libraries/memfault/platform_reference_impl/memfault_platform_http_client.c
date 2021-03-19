//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Reference implementation of platform dependencies for the Memfault HTTP Client when using the
//! WICED SDK

#include "memfault/http/platform/http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/math.h"
#include "memfault/panics/assert.h"
#include "memfault_platform_wiced.h"

#include "http.h"
#include "http_client.h"
#include "wiced_management.h"
#include "wiced_tls.h"
#include "wwd_debug.h"

#define MEMFAULT_DNS_TIMEOUT_MS (10000)
#define MEMFAULT_HTTP_CONNECT_TIMEOUT_MS (10000)
#define MEMFAULT_HTTP_POST_DATA_READ_BUFFER_SIZE (256)
#define MEMFAULT_HTTP_MIME_TYPE_BINARY "application/octet-stream"
// API expects colon+space in fieldname :/
#define MEMFAULT_HTTP_PROJECT_KEY_HEADER_WICED MEMFAULT_HTTP_PROJECT_KEY_HEADER ": "

typedef struct MfltHttpClient {
  http_client_t client;
  http_client_configuration_info_t config;
  bool is_dns_lookup_done;
  wiced_ip_address_t ip_address;

  bool is_request_pending;
  bool request_error_occurred;
  http_request_t request;
  MemfaultHttpClientResponseCallback callback;

  void *callback_ctx;
} sMfltHttpClient;

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response, uint32_t *status_out) {
  MEMFAULT_ASSERT(response);
  http_response_t *wiced_response = (http_response_t *)response;
  http_status_line_t status_line = {0};
  if (WICED_SUCCESS != http_get_status_line(
      wiced_response->response_hdr, wiced_response->response_hdr_length, &status_line)) {
    return -1;
  }
  if (status_out) {
    *status_out = status_line.code;
  }
  return 0;
}

static void prv_finalize_request_and_run_callback(sMfltHttpClient *client, http_response_t *response) {
  if (!client->is_request_pending) {
    return;
  }
  if (client->callback) {
    client->callback((const sMfltHttpResponse *) response, client->callback_ctx);
  }

  uint32_t http_status = 0;
  const int rv = memfault_platform_http_response_get_status((const sMfltHttpResponse *)response,
                                                            &http_status);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("Request failed. No HTTP status: %d", rv);
    return;
  }
  client->request_error_occurred = (http_status < 200) || (http_status >= 300);
  client->callback = NULL;
  client->callback_ctx = NULL;
  http_request_deinit(&client->request);
  client->is_request_pending = false;
}

static void prv_handle_data_received(sMfltHttpClient *client, http_response_t *response) {
  if (response->request != &client->request) {
    MEMFAULT_LOG_DEBUG("Recv data for different req");
    return;
  }
  if (response->response_hdr == NULL) {
    // Will it come later?
    MEMFAULT_LOG_DEBUG("NULL header");
    return;
  }
  http_status_line_t status_line;
  if (WICED_SUCCESS != http_get_status_line(response->response_hdr, response->response_hdr_length, &status_line)) {
    MEMFAULT_LOG_DEBUG("Couldn't parse status line");
    return;
  }
  prv_finalize_request_and_run_callback(client, response);
}

static void prv_http_event_handler(http_client_t *wiced_client, http_event_t event, http_response_t *response) {
  MEMFAULT_STATIC_ASSERT(offsetof(sMfltHttpClient, client) == 0, "Expecting first member to be http_client_t client");
  sMfltHttpClient *client = (sMfltHttpClient *)wiced_client;

  switch (event) {
    case HTTP_CONNECTED:
      break;

    case HTTP_DISCONNECTED: {
      prv_finalize_request_and_run_callback(client, NULL);
      break;
    }

    case HTTP_DATA_RECEIVED: {
      prv_handle_data_received(client, response);
      break;
    }

    default:
      break;
  }
}

sMfltHttpClient *memfault_platform_http_client_create(void) {
  if (wiced_network_is_up(WICED_STA_INTERFACE) == WICED_FALSE) {
    // If the network is not up (WiFi is not joined) http_client_init() will trip an assert... :/ Race prone?
    goto error;
  }
  sMfltHttpClient *client = malloc(sizeof(sMfltHttpClient));
  if (!client) {
    goto error;
  }
  memset(client, 0, sizeof(*client));

  if (WICED_SUCCESS != http_client_init(&client->client, WICED_STA_INTERFACE, prv_http_event_handler, NULL)) {
    goto error;
  }

  const char *hostname = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();
  client->config = (http_client_configuration_info_t) {
      .flag = (http_client_configuration_flags_t)(
          HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME | HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN),
      .server_name = (uint8_t *)hostname,
      .max_fragment_length = TLS_FRAGMENT_LENGTH_1024,
  };
  if (WICED_SUCCESS != http_client_configure(&client->client, &client->config)) {
    http_client_deinit(&client->client);
    goto error;
  }

  /* if you set hostname, library will make sure subject name in the server certificate is matching with host name
   * you are trying to connect. pass NULL if you don't want to enable this check */
  client->client.peer_cn = (uint8_t *)hostname;

  return client;

error:
  free(client);
  return NULL;
}

static bool prv_do_dns_lookup(sMfltHttpClient *client) {
  if (client->is_dns_lookup_done) {
    return true;
  }
  const wiced_result_t dns_rv = wiced_hostname_lookup((const char *)client->config.server_name, &client->ip_address,
      MEMFAULT_DNS_TIMEOUT_MS, WICED_STA_INTERFACE);
  if (WICED_SUCCESS != dns_rv) {
    MEMFAULT_LOG_ERROR("DNS lookup failed: %d", dns_rv);
    return false;
  }
  client->is_dns_lookup_done = true;
  return true;
}

static wiced_result_t prv_send_chunk_in_http_request(
    sMfltHttpClient *client, const char *url,
    MemfaultHttpClientResponseCallback callback, void *ctx) {
  uint8_t *buffer = malloc(MEMFAULT_HTTP_POST_DATA_READ_BUFFER_SIZE);
  if (!buffer) {
    MEMFAULT_LOG_ERROR("%s: malloc fail", __func__);
    return WICED_ERROR;
  }

  sPacketizerMetadata metadata;
  const sPacketizerConfig cfg = {
    // We will send an entire memfault data message in a single http request
    .enable_multi_packet_chunk = true,
  };
  bool md = memfault_packetizer_begin(&cfg, &metadata);
  if (!md) {
    // we should only be in this function when there _is_ data available
    goto error;
  }

  char content_length[12] = {0};
  snprintf(content_length, sizeof(content_length), "%d",
           (int)metadata.single_chunk_message_length);

  const http_header_field_t headers[] = {
      [0] = {
          .field = HTTP_HEADER_HOST,
          .field_length = sizeof(HTTP_HEADER_HOST) - 1,
          .value = (char *)client->config.server_name,
          .value_length = strlen((const char *)client->config.server_name),
      },
      [1] = {
          .field = MEMFAULT_HTTP_PROJECT_KEY_HEADER_WICED,
          .field_length = sizeof(MEMFAULT_HTTP_PROJECT_KEY_HEADER_WICED) - 1,
          .value = (char *)g_mflt_http_client_config.api_key,
          .value_length = strlen((const char *)g_mflt_http_client_config.api_key),
      },
      [2] = {
          .field = HTTP_HEADER_CONTENT_LENGTH,
          .field_length = sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1,
          .value = content_length,
          .value_length = strlen(content_length),
      },
      [3] = {
          .field = HTTP_HEADER_CONTENT_TYPE,
          .field_length = sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,
          .value = MEMFAULT_HTTP_MIME_TYPE_BINARY,
          .value_length = sizeof(MEMFAULT_HTTP_MIME_TYPE_BINARY) - 1,
      }
  };

  http_request_t *const request = &client->request;

  WICED_VERIFY(http_request_init(request, &client->client, HTTP_POST, url, HTTP_1_1));
  WICED_VERIFY(http_request_write_header(request, headers, MEMFAULT_ARRAY_SIZE(headers)));
  WICED_VERIFY(http_request_write_end_header(request));

  eMemfaultPacketizerStatus packetizer_status;
  do {
    size_t buffer_len = MEMFAULT_HTTP_POST_DATA_READ_BUFFER_SIZE;
    packetizer_status =
        memfault_packetizer_get_next(buffer, &buffer_len);
    if (packetizer_status == kMemfaultPacketizerStatus_NoMoreData) {
      break;
    }

    const wiced_result_t write_rv = http_request_write(request, buffer, buffer_len);
    if (WICED_SUCCESS != write_rv) {
      MEMFAULT_LOG_ERROR("http_request_write failed: %u", write_rv);
      goto error;
    }
  } while (packetizer_status != kMemfaultPacketizerStatus_EndOfChunk);

  // we have finished assembling one chunk so flush the request
  const wiced_result_t flush_rv = http_request_flush(request);
  if (WICED_SUCCESS != flush_rv) {
    MEMFAULT_LOG_ERROR("http_request_flush failed: %u", flush_rv);
    goto error;
  }

  free(buffer);
  client->callback = callback;
  client->callback_ctx = ctx;
  client->is_request_pending = true;
  return WICED_SUCCESS;

error:
  http_request_deinit(&client->request);
  free(buffer);
  return WICED_ERROR;
}

// return different error codes from each exit point so it's easier to determine what went wrong
typedef enum {
  kMemfaultPlatformHttpPost_AlreadyPending = -1,
  kMemfaultPlatformHttpPost_GetSpiAddressFailed = -2,
  kMemfaultPlatformHttpPost_DnsLookupFailed = -3,
  kMemfaultPlatformHttpPost_BuildUrlFailed = -4,
  kMemfaultPlatformHttpPost_HttpRequestFailure = -5,
} eMemfaultPlatformHttpPost;

int memfault_platform_http_client_post_data(
    sMfltHttpClient *client, MemfaultHttpClientResponseCallback callback, void *ctx) {
  if (client->is_request_pending) {
    MEMFAULT_LOG_ERROR("Data post request already pending!");
    return kMemfaultPlatformHttpPost_AlreadyPending;
  }

  // Early exit if there is no new data to send
  if (!memfault_packetizer_data_available()) {
    return kMfltPostDataStatus_NoDataFound;
  }

  if (!prv_do_dns_lookup(client)) {
    return kMemfaultPlatformHttpPost_DnsLookupFailed;
  }

  char url_buffer[MEMFAULT_HTTP_URL_BUFFER_SIZE];
  if (!memfault_http_build_url(url_buffer, MEMFAULT_HTTP_CHUNKS_API_SUBPATH)) {
    return kMemfaultPlatformHttpPost_BuildUrlFailed;
  }
  MEMFAULT_LOG_DEBUG("Posting data to %s", url_buffer);

  const http_security_t security = g_mflt_http_client_config.disable_tls ? HTTP_NO_SECURITY : HTTP_USE_TLS;
  WICED_VERIFY(http_client_connect(&client->client, &client->ip_address, MEMFAULT_HTTP_GET_CHUNKS_API_PORT(),
                                   security, MEMFAULT_HTTP_CONNECT_TIMEOUT_MS));

  // Drain all the data that is available to be sent
  while (memfault_packetizer_data_available()) {
    int rv = prv_send_chunk_in_http_request(client, url_buffer, callback, ctx);
    if (rv != WICED_SUCCESS) {
      return MEMFAULT_PLATFORM_SPECIFIC_ERROR(rv);
    }

    // Wait for the in-flight http request to complete
    memfault_platform_http_client_wait_until_requests_completed(
        client, MEMFAULT_HTTP_CONNECT_TIMEOUT_MS);

    if (client->request_error_occurred) {
      MEMFAULT_LOG_ERROR("Terminating data transfer because error occurred");
      memfault_packetizer_abort();
      return kMemfaultPlatformHttpPost_HttpRequestFailure;
    }
  }

  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  uint32_t waited_ms = 0;
  while (client->is_request_pending) {
    // Could also be implemented using a semaphore
    wiced_rtos_delay_milliseconds(100);
    waited_ms += 100;
    if (waited_ms >= timeout_ms) {
      return -1;
    }
  }
  return 0;
}

int memfault_platform_http_client_destroy(sMfltHttpClient *client) {
  http_client_deinit((http_client_t *)client);
  free(client);
  return 0;
}
