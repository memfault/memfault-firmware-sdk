//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Example implementation of platform dependencies on the ESP32 for the Memfault HTTP APIs

#include "memfault/http/platform/http_client.h"

#include <string.h>

#include "esp_http_client.h"
#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/errors.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault/esp_port/http_client.h"
#include "memfault/http/http_client.h"
#include "memfault/http/root_certs.h"
#include "memfault/panics/assert.h"

#ifndef MEMFAULT_HTTP_DEBUG
#  define MEMFAULT_HTTP_DEBUG (0)
#endif

#if MEMFAULT_HTTP_DEBUG
static esp_err_t prv_http_event_handler(esp_http_client_event_t *evt) {
  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      memfault_platform_log(kMemfaultPlatformLogLevel_Error, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info,
          "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info,
          "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // memfault_platform_log(kMemfaultPlatformLogLevel_Info, "%.*s", evt->data_len, (char*)evt->data);
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info, "HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}
#endif  // MEMFAULT_HTTP_DEBUG



static int prv_post_chunks(esp_http_client_handle_t client, void *buffer, size_t buf_len) {
  // drain all the chunks we have
  while (1) {
    // NOTE: Ideally we would be able to enable multi packet chunking which would allow a chunk to
    // span multiple calls to memfault_packetizer_get_next(). Unfortunately the esp-idf does not
    // have a POST mechanism that can use a callback so our POST size is limited by the size of the
    // buffer we can allocate.
    size_t read_size = buf_len;
    bool more_data = memfault_packetizer_get_chunk(buffer, &read_size);
    if (!more_data) {
      break;
    }

    esp_http_client_set_post_field(client, buffer, read_size);
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

    esp_err_t err = esp_http_client_perform(client);
    if (ESP_OK != err) {
      return MEMFAULT_PLATFORM_SPECIFIC_ERROR(err);
    }
  }

  return 0;
}

static char s_mflt_base_url_buffer[MEMFAULT_HTTP_URL_BUFFER_SIZE];

sMfltHttpClient *memfault_platform_http_client_create(void) {
  memfault_http_build_url(s_mflt_base_url_buffer, "");
  const esp_http_client_config_t config = {
#if MEMFAULT_HTTP_DEBUG
    .event_handler = prv_http_event_handler,
#endif
    .url = s_mflt_base_url_buffer,
    .cert_pem = g_mflt_http_client_config.disable_tls ? NULL : MEMFAULT_ROOT_CERTS_PEM,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_header(client, MEMFAULT_HTTP_PROJECT_KEY_HEADER, g_mflt_http_client_config.api_key);
  esp_http_client_set_header(client, "Accept", "application/json");
  return (sMfltHttpClient *)client;
}

int memfault_platform_http_client_destroy(sMfltHttpClient *_client) {
  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  esp_err_t err = esp_http_client_cleanup(client);
  if (err == ESP_OK) {
    return 0;
  }

  return MEMFAULT_PLATFORM_SPECIFIC_ERROR(err);
}

typedef struct MfltHttpResponse {
  uint16_t status;
} sMfltHttpResponse;

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response, uint32_t *status_out) {
  MEMFAULT_ASSERT(response);
  if (status_out) {
    *status_out = response->status;
  }
  return 0;
}

int memfault_platform_http_client_post_data(
    sMfltHttpClient *_client, MemfaultHttpClientResponseCallback callback, void *ctx) {

  if (!memfault_packetizer_data_available()) {
    return 0; // no new chunks to send
  }

  MEMFAULT_LOG_DEBUG("Posting Memfault Data");

  size_t buffer_size = 0;
  void *buffer = memfault_http_client_allocate_chunk_buffer(&buffer_size);
  if (buffer == NULL || buffer_size == 0) {
    MEMFAULT_LOG_ERROR("Unable to allocate POST buffer");
    return -1;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  char url[MEMFAULT_HTTP_URL_BUFFER_SIZE];
  memfault_http_build_url(url, MEMFAULT_HTTP_CHUNKS_API_SUBPATH);
  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_POST);

  int rv = prv_post_chunks(client, buffer, buffer_size);
  memfault_http_client_release_chunk_buffer(buffer);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("%s failed: %d", __func__, (int)rv);
    return rv;
  }

  const sMfltHttpResponse response = {
    .status = (uint32_t)esp_http_client_get_status_code(client),
  };
  if (callback) {
    callback(&response, ctx);
  }
  MEMFAULT_LOG_DEBUG("Posting Memfault Data Complete!");
  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  // No-op because memfault_platform_http_client_post_data() is synchronous
  return 0;
}
