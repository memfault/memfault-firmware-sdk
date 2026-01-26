//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Example implementation of platform dependencies on the ESP32 for the Memfault HTTP APIs

#include "memfault/config.h"

#if MEMFAULT_ESP_HTTP_CLIENT_ENABLE

  #include <stdio.h>
  #include <string.h>

  #include "esp_http_client.h"
  #include "esp_https_ota.h"
  #include "esp_idf_version.h"
  #include "esp_wifi.h"
  #include "memfault/components.h"
  #include "memfault/esp_port/core.h"
  #include "memfault/esp_port/http_client.h"
  #include "memfault/version.h"

  #ifndef MEMFAULT_HTTP_DEBUG
    #define MEMFAULT_HTTP_DEBUG (0)
  #endif

  //! Default buffer size for the URL-encoded device info parameters. This may
  //! need to be set higher by the user if there are particularly long device info
  //! strings
  #ifndef MEMFAULT_DEVICE_INFO_URL_ENCODED_MAX_LEN
    #define MEMFAULT_DEVICE_INFO_URL_ENCODED_MAX_LEN (48)
  #endif

  #define MEMFAULT_HTTP_USER_AGENT "MemfaultSDK/" MEMFAULT_SDK_VERSION_STR

  #if CONFIG_MEMFAULT_HTTP_MAX_REQUEST_SIZE > 0 && \
    (CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN < (CONFIG_MEMFAULT_HTTP_MAX_REQUEST_SIZE + 1024))
    #warning \
      "MBEDTLS_SSL_IN_CONTENT_LEN < CONFIG_MEMFAULT_HTTP_MAX_REQUEST_SIZE + 1kB recommended room for TLS overhead"
  #endif

MEMFAULT_STATIC_ASSERT(
  sizeof(CONFIG_MEMFAULT_PROJECT_KEY) > 1,
  "Memfault Project Key not configured. Please visit https://mflt.io/project-key "
  "and add CONFIG_MEMFAULT_PROJECT_KEY=\"YOUR_KEY\" to sdkconfig.defaults");

sMfltHttpClientConfig g_mflt_http_client_config = { .api_key = CONFIG_MEMFAULT_PROJECT_KEY };

// Track the number of bytes sent in each data post operation
size_t g_chunk_bytes_sent;

  #if MEMFAULT_HTTP_DEBUG
static esp_err_t prv_http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
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
                            "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
                            evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      memfault_platform_log(kMemfaultPlatformLogLevel_Info, "HTTP_EVENT_ON_DATA, len=%d",
                            evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // memfault_platform_log(kMemfaultPlatformLogLevel_Info, "%.*s", evt->data_len,
        // (char*)evt->data);
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

static int prv_post_chunks(esp_http_client_handle_t client, void *buffer, size_t buf_len,
                           size_t *chunk_bytes_sent) {
  *chunk_bytes_sent = 0;
  // drain all the chunks we have
  while (1) {
    // NOTE: Ideally we would be able to enable multi packet chunking which would allow a chunk to
    // span multiple calls to memfault_packetizer_get_next(). Unfortunately the esp-idf does not
    // have a POST mechanism that can use a callback so our POST size is limited by the size of the
    // buffer we can allocate.
    size_t read_size = buf_len;

    const bool more_data = memfault_esp_port_get_chunk(buffer, &read_size);
    if (!more_data) {
      break;
    }

    *chunk_bytes_sent += read_size;

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
    .timeout_ms = CONFIG_MEMFAULT_HTTP_CLIENT_TIMEOUT_MS,
    .cert_pem = g_mflt_http_client_config.disable_tls ? NULL : MEMFAULT_ROOT_CERTS_PEM,
    .user_agent = MEMFAULT_HTTP_USER_AGENT,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    return NULL;
  }
  esp_http_client_set_header(client, MEMFAULT_HTTP_PROJECT_KEY_HEADER,
                             g_mflt_http_client_config.api_key);
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

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response,
                                               uint32_t *status_out) {
  MEMFAULT_ASSERT(response);
  if (status_out) {
    *status_out = response->status;
  }
  return 0;
}

//! Check the 3 device info fields that aren't escaped. Return -1 if any need
//! escaping
static int prv_deviceinfo_needs_url_escaping(sMemfaultDeviceInfo *device_info) {
  const struct params_s {
    const char *name;
    const char *value;
  } params[] = {
    {
      .name = "device_serial",
      .value = device_info->device_serial,
    },
    {
      .name = "hardware_version",
      .value = device_info->hardware_version,
    },
    {
      .name = "software_type",
      .value = device_info->software_type,
    },
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(params); i++) {
    if (memfault_http_needs_escape(params[i].value, strlen(params[i].value))) {
      MEMFAULT_LOG_ERROR(
        "OTA URL query param '%s' contains reserved characters and needs escaping: %s",
        params[i].name, params[i].value);
      return -1;
    }
  }

  return 0;
}

static int prv_build_latest_release_url(char *buf, size_t buf_len) {
  sMemfaultDeviceInfo device_info;
  memfault_http_get_device_info(&device_info);

  // if any of the device info fields need escaping, abort
  if (prv_deviceinfo_needs_url_escaping(&device_info)) {
    return -1;
  }

  // URL encode software_version before appending it to the URL; most likely to
  // have reserved characters
  char urlencoded_software_version[MEMFAULT_DEVICE_INFO_URL_ENCODED_MAX_LEN];
  int rv =
    memfault_http_urlencode(device_info.software_version, strlen(device_info.software_version),
                            urlencoded_software_version, sizeof(urlencoded_software_version));
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("Failed to URL encode software version");
    return -1;
  }

  return snprintf(buf, buf_len,
                  "%s://%s/api/v0/releases/latest/"
                  "url?device_serial=%s&hardware_version=%s&software_type=%s&current_version=%s",
                  MEMFAULT_HTTP_GET_SCHEME(), MEMFAULT_HTTP_GET_DEVICE_API_HOST(),
                  device_info.device_serial, device_info.hardware_version,
                  device_info.software_type, urlencoded_software_version);
}

int memfault_esp_port_ota_get_release_url(char **download_url_out) {
  sMfltHttpClient *http_client = memfault_http_client_create();
  char *url = NULL;
  char *download_url = NULL;
  int status_code = -1;

  int rv = -1;
  if (http_client == NULL) {
    MEMFAULT_LOG_ERROR("OTA: failed to create HTTP client");
    return rv;
  }

  // call once with no buffer to figure out space we need to allocate to hold url
  int url_len = prv_build_latest_release_url(NULL, 0);
  if (url_len < 0) {
    MEMFAULT_LOG_ERROR("Failed to build OTA URL");
    goto cleanup;
  }

  const size_t url_buf_len = url_len + 1 /* for '\0' */;
  url = calloc(1, url_buf_len);
  if (url == NULL) {
    MEMFAULT_LOG_ERROR("Unable to allocate url buffer (%dB)", (int)url_buf_len);
    goto cleanup;
  }

  if (prv_build_latest_release_url(url, url_buf_len) != url_len) {
    goto cleanup;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)http_client;

  // NB: For esp-idf versions > v3.3 will set the Host automatically as part
  // of esp_http_client_set_url() so this call isn't strictly necessary.
  //
  //  https://github.com/espressif/esp-idf/commit/a8755055467f3e6ab44dd802f0254ed0281059cc
  //  https://github.com/espressif/esp-idf/commit/d154723a840f04f3c216df576456830c884e7abd
  esp_http_client_set_header(client, "Host", MEMFAULT_HTTP_GET_DEVICE_API_HOST());

  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  // to keep the parsing simple, we will request the download url as plaintext
  esp_http_client_set_header(client, "Accept", "text/plain");

  const esp_err_t err = esp_http_client_open(client, 0);
  if (ESP_OK != err) {
    rv = MEMFAULT_PLATFORM_SPECIFIC_ERROR(err);
    goto cleanup;
  }

  const int content_length = esp_http_client_fetch_headers(client);
  if (content_length < 0) {
    rv = MEMFAULT_PLATFORM_SPECIFIC_ERROR(content_length);
    goto cleanup;
  }

  download_url = calloc(1, content_length + 1);
  if (download_url == NULL) {
    MEMFAULT_LOG_ERROR("Unable to allocate download url buffer (%dB)", (int)download_url);
    goto cleanup;
  }

  int bytes_read = 0;
  while (bytes_read != content_length) {
    int len = esp_http_client_read(client, &download_url[bytes_read], content_length - bytes_read);
    if (len < 0) {
      rv = MEMFAULT_PLATFORM_SPECIFIC_ERROR(len);
      goto cleanup;
    }
    bytes_read += len;
  }

  status_code = esp_http_client_get_status_code(client);
  if (status_code != 200 && status_code != 204) {
    MEMFAULT_LOG_ERROR("OTA Query Failure. Status Code: %d", status_code);
    MEMFAULT_LOG_INFO("Response Body: %s", download_url);
    goto cleanup;
  }

  // Lookup to see if a release is available was successful!
  rv = 0;

cleanup:
  free(url);
  memfault_http_client_destroy(http_client);

  if (status_code == 200) {
    *download_url_out = download_url;
  } else {
    // on failure or if no update is available (204) there is no download url to return
    free(download_url);
    *download_url_out = NULL;
  }
  return rv;
}

int memfault_esp_port_ota_update(const sMemfaultOtaUpdateHandler *handler) {
  char *download_url = NULL;
  int rv;

  if ((handler == NULL) || (handler->handle_update_available == NULL) ||
      (handler->handle_download_complete == NULL)) {
    rv = MemfaultInternalReturnCode_InvalidInput;
    goto cleanup;
  }

  rv = memfault_esp_port_ota_get_release_url(&download_url);

  if ((rv != 0) || (download_url == NULL)) {
    goto cleanup;
  }

  printf("Download URL: %s\n", download_url);

  const bool perform_ota = handler->handle_update_available(handler->user_ctx);
  if (!perform_ota) {
    // Client decided to abort the OTA but we still set the return code to 1 to indicate a new
    // update is available.
    rv = 1;
    goto cleanup;
  }

  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_https_ota_config_t config = {
    .http_config =
      &(esp_http_client_config_t){
        .url = download_url,
        .timeout_ms = CONFIG_MEMFAULT_HTTP_CLIENT_TIMEOUT_MS,
        .cert_pem = MEMFAULT_ROOT_CERTS_PEM,
        .user_agent = MEMFAULT_HTTP_USER_AGENT,
      },
    .http_client_init_cb = NULL,
    .bulk_flash_erase = false,
    // The following 2 config options were made optional in ESP-IDF v6
    #if (ESP_IDF_VERSION <= ESP_IDF_VERSION_VAL(5, 5, 0)) || \
      defined(CONFIG_ESP_HTTPS_OTA_ENABLE_PARTIAL_DOWNLOAD)
      #if defined(CONFIG_MEMFAULT_HTTP_PARTIAL_DOWNLOAD_ENABLE)
    .partial_http_download = true,
      #else
    .partial_http_download = false,
      #endif
    .max_http_request_size = CONFIG_MEMFAULT_HTTP_MAX_REQUEST_SIZE,
    #endif
  };
  #else
  esp_http_client_config_t config = {
    .url = download_url,
    .timeout_ms = CONFIG_MEMFAULT_HTTP_CLIENT_TIMEOUT_MS,
    .cert_pem = MEMFAULT_ROOT_CERTS_PEM,
  };
  #endif

  const esp_err_t err = esp_https_ota(&config);
  if (err != ESP_OK) {
    rv = MEMFAULT_PLATFORM_SPECIFIC_ERROR(err);
    goto cleanup;
  }

  const bool success = handler->handle_download_complete(handler->user_ctx);
  rv = success ? 1 : -1;

cleanup:
  free(download_url);

  if (handler->handle_ota_done) {
    handler->handle_ota_done(rv, handler->user_ctx);
  }
  return rv;
}

int memfault_platform_http_client_post_data(sMfltHttpClient *_client,
                                            MemfaultHttpClientResponseCallback callback,
                                            void *ctx) {
  if (!memfault_esp_port_data_available()) {
    return 0;  // no new chunks to send
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
  esp_http_client_set_header(client, "Accept", "application/json");

  int rv = prv_post_chunks(client, buffer, buffer_size, &g_chunk_bytes_sent);
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
  if ((response.status == 200) || (response.status == 202)) {
    return 0;
  } else {
    // Use #define MEMFAULT_HTTP_DEBUG=1 to enable response payload debug log
    return -1;
  }
}

int memfault_platform_http_client_wait_until_requests_completed(sMfltHttpClient *client,
                                                                uint32_t timeout_ms) {
  // No-op because memfault_platform_http_client_post_data() is synchronous
  return 0;
}

bool memfault_esp_port_wifi_connected(void) {
  wifi_ap_record_t ap_info;
  const bool connected = esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
  return connected;
}

bool memfault_esp_port_netif_connected(void) {
  // iterate over all netifs and check if any of them are connected
  esp_netif_t *netif = NULL;
  while ((netif =
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
            esp_netif_next_unsafe
  #else
            esp_netif_next
  #endif
          (netif)) != NULL) {
    if (!esp_netif_is_netif_up(netif)) {
      continue;
    }
    esp_netif_ip_info_t ip_info;
    esp_err_t err = esp_netif_get_ip_info(netif, &ip_info);
    if ((err == ESP_OK) && (ip_info.ip.addr != 0)) {
      return true;
    }
  }

  // didn't find a netif with a non-zero IP address, return not connected
  return false;
}

// Similar to memfault_platform_http_client_post_data() but just posts
// whatever is pending, if anything.
int memfault_esp_port_http_client_post_data(void) {
  if (!memfault_esp_port_netif_connected()) {
    MEMFAULT_LOG_INFO("%s: Network unavailable", __func__);
    return -1;
  }

  // Check for data available first as nothing else matters if not.
  if (!memfault_esp_port_data_available()) {
    MEMFAULT_LOG_INFO("No new data found");
    return 0;
  }

  sMfltHttpClient *http_client = memfault_http_client_create();
  if (!http_client) {
    MEMFAULT_LOG_ERROR("Failed to create HTTP client");
  #if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
    // count as a sync failure- if we're here, we have a valid netif connection
    // but were unable to establish the TLS connection
    memfault_metrics_connectivity_record_memfault_sync_failure();
  #endif
    return MemfaultInternalReturnCode_Error;
  }
  const eMfltPostDataStatus rv = (eMfltPostDataStatus)memfault_http_client_post_data(http_client);
  switch (rv) {
    case kMfltPostDataStatus_Success:
      MEMFAULT_LOG_INFO("Data posted successfully, %d bytes sent", (int)g_chunk_bytes_sent);
  #if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
      memfault_metrics_connectivity_record_memfault_sync_success();
  #endif
      break;

    case kMfltPostDataStatus_NoDataFound:
      MEMFAULT_LOG_INFO("Done: no data to send");
      break;

    default:
      MEMFAULT_LOG_ERROR("Failed to post data, err=%d", rv);
  #if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
      memfault_metrics_connectivity_record_memfault_sync_failure();
  #endif
      break;
  }

  const uint32_t timeout_ms = 30 * 1000;
  memfault_http_client_wait_until_requests_completed(http_client, timeout_ms);
  memfault_http_client_destroy(http_client);
  return (int)rv;
}

#endif /* MEMFAULT_ESP_HTTP_CLIENT_ENABLE */
