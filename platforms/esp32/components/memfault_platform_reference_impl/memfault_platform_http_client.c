#include "memfault/http/platform/http_client.h"

#include "memfault/panics/assert.h"
#include "memfault/panics/coredump_impl.h"
#include "memfault/http/http_client.h"
#include "memfault/core/math.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/core/platform/debug_log.h"
#include "memfault_platform_coredump_esp32.h"

#include "esp_http_client_mflt.h"

#include <string.h>

#ifndef MEMFAULT_HTTP_DEBUG
#  define MEMFAULT_HTTP_DEBUG (0)
#endif
#define MEMFAULT_HTTP_POST_COREDUMP_READ_BUFFER_SIZE (256)

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

static bool prv_calc_block_size(const void *data, size_t len, void *ctx) {
  size_t *total_size = ctx;
  *total_size += len;
  return true;
}

typedef struct MfltCoredumpPostUserData {
  size_t esp_idf_coredump_size;
  // Size of the coredump payload, *excluding* the sMfltCoredumpHeader header:
  size_t request_body_size;
} sMfltCoredumpPostUserData;

static void prv_write_all_blocks(size_t total_size, void *esp_idf_coredump_payload, size_t esp_idf_coredump_size,
                                 MfltCoredumpWriteCb write_cb, void *ctx) {
  memfault_coredump_write_header(total_size, write_cb, ctx);
  memfault_coredump_write_device_info_blocks(write_cb, ctx);
  memfault_coredump_write_block(kMfltCoredumpRegionType_VendorCoredumpEspIdfV2ToV3_1,
      esp_idf_coredump_payload, esp_idf_coredump_size, write_cb, ctx);
}

static void prv_calc_sizes(sMfltCoredumpPostUserData *size_info) {
  size_info->request_body_size = 0;
  // Doesn't matter what we pass as esp_idf_coredump_payload, as long as it's not NULL,
  // otherwise the write_cb won't be invoked to account for the size of the payload:
  void *dummy_esp_idf_coredump_payload = size_info;
  prv_write_all_blocks(0, dummy_esp_idf_coredump_payload, size_info->esp_idf_coredump_size,
                       prv_calc_block_size, &size_info->request_body_size);
}

static bool prv_write_to_http_client(const void *data, size_t len, void *ctx) {
  esp_http_client_handle_t client = ctx;
  esp_http_client_write(client, data, len);
  return true;
}

static void prv_write_coredump_request_body(esp_http_client_handle_t client) {
  const sMfltCoredumpPostUserData *size_info = esp_http_client_get_user_data(client);

  // Pass NULL as the block payload, so only the header gets written. We'll write the block payload in the loop below.
  prv_write_all_blocks(size_info->request_body_size, NULL, size_info->esp_idf_coredump_size,
      prv_write_to_http_client, client);

  size_t bytes_remaining = size_info->esp_idf_coredump_size;
  char *buffer = malloc(MEMFAULT_HTTP_POST_COREDUMP_READ_BUFFER_SIZE);
  if (!buffer) {
    memfault_platform_log(kMemfaultPlatformLogLevel_Error, "malloc fail");
    return;
  }
  while (bytes_remaining) {
    const size_t offset = size_info->esp_idf_coredump_size - bytes_remaining;
    const size_t read_size = MEMFAULT_MIN(bytes_remaining, MEMFAULT_HTTP_POST_COREDUMP_READ_BUFFER_SIZE);
    if (!memfault_platform_coredump_storage_read(offset, buffer, read_size)) {
      break;
    }
    esp_http_client_write(client, buffer, read_size);
    bytes_remaining -= read_size;
  }
  free(buffer);
}

static char s_mflt_base_url_buffer[MEMFAULT_HTTP_URL_BUFFER_SIZE];

sMfltHttpClient *memfault_platform_http_client_create(void) {
  memfault_http_build_url(s_mflt_base_url_buffer, "");
  const esp_http_client_config_t config = {
#if MEMFAULT_HTTP_DEBUG
      .event_handler = prv_http_event_handler,
#endif
      .url = s_mflt_base_url_buffer,
      // TODO: Add CA root certs for TLS
      // .cert_pem = howsmyssl_com_root_cert_pem_start,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_header(client, MEMFAULT_HTTP_PROJECT_KEY_HEADER, g_mflt_http_client_config.api_key);
  esp_http_client_set_header(client, "Accept", "application/json");
  return (sMfltHttpClient *)client;
}

MemfaultReturnCode memfault_platform_http_client_destroy(sMfltHttpClient *_client) {
  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  return (ESP_OK == esp_http_client_cleanup(client)) ? MemfaultReturnCode_Ok : MemfaultReturnCode_Error;
}

typedef struct MfltHttpResponse {
  uint16_t status;
} sMfltHttpResponse;

MemfaultReturnCode memfault_platform_http_response_get_status(const sMfltHttpResponse *response, uint32_t *status_out) {
  MEMFAULT_ASSERT(response);
  if (status_out) {
    *status_out = response->status;
  }
  return MemfaultReturnCode_Ok;
}

MemfaultReturnCode memfault_platform_http_client_post_coredump(sMfltHttpClient *_client,
                                                               MemfaultHttpClientResponseCallback callback, void *ctx) {
  sMfltCoredumpPostUserData user_data;
  if (!memfault_coredump_get_esp_idf_formatted_coredump_size(&user_data.esp_idf_coredump_size)) {
    return MemfaultReturnCode_DoesNotExist;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  prv_calc_sizes(&user_data);
  esp_http_client_set_user_data(client, &user_data);

  char url[MEMFAULT_HTTP_URL_BUFFER_SIZE];
  memfault_http_build_url(url, MEMFAULT_HTTP_API_COREDUMP_SUBPATH);
  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
  esp_http_client_set_request_body_handler(client, prv_write_coredump_request_body, user_data.request_body_size);

  esp_err_t err = esp_http_client_perform(client);
  if (ESP_OK != err) {
    return MemfaultReturnCode_Error;
  }

  // Cleanup request-specific headers we've added:
  esp_http_client_delete_header(client, "Content-Type");

  const sMfltHttpResponse response = {
      .status = (uint32_t)esp_http_client_get_status_code(client),
  };
  if (callback) {
    callback(&response, ctx);
  }
  return MemfaultReturnCode_Ok;
}

MemfaultReturnCode memfault_platform_http_client_wait_until_requests_completed(
    sMfltHttpClient *client, uint32_t timeout_ms) {
  // No-op because memfault_platform_http_client_post_coredump() is synchronous
  return MemfaultReturnCode_Ok;
}
