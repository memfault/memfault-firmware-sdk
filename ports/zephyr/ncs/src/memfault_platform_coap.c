//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include <stdio.h>
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/coap.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/coap_client.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/socket.h)
#include <date_time.h>
#include <modem/modem_jwt.h>
#include <net/nrf_cloud_coap.h>
#include <nrf_cloud_coap_transport.h>
#include <nrf_modem_at.h>
#include <zephyr/random/random.h>

#include "memfault/components.h"
#include "memfault/nrfconnect_port/coap.h"

//! Default buffer size for proxy URLS. This may need to be increased by the user if there are
//! particularly long device properties (serial, hardware version, software version, or type)
#ifndef MEMFAULT_PROXY_URL_MAX_LEN
  #define MEMFAULT_PROXY_URL_MAX_LEN (200)
#endif

// Ensure MEMFAULT_COAP_MAX_POST_SIZE doesn't exceed available payload space
// MAX_COAP_MSG_LEN = COAP_CLIENT_MESSAGE_HEADER_SIZE + COAP_CLIENT_MESSAGE_SIZE
// Available payload size = COAP_CLIENT_MESSAGE_HEADER_SIZE + COAP_CLIENT_MESSAGE_SIZE -
// conservative 100 B options_size
#if CONFIG_MEMFAULT_COAP_MAX_POST_SIZE > \
  (CONFIG_COAP_CLIENT_MESSAGE_SIZE + CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE - 100)
  #error \
    "CONFIG_MEMFAULT_COAP_MAX_POST_SIZE exceeds available payload space. Increase CONFIG_COAP_CLIENT_MESSAGE_SIZE or reduce CONFIG_MEMFAULT_COAP_MAX_POST_SIZE."
#endif

#if (CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE > 0)
static void *prv_calloc(size_t count, size_t size) {
  return calloc(count, size);
}

static void prv_free(void *ptr) {
  free(ptr);
}
#elif CONFIG_HEAP_MEM_POOL_SIZE > 0
static void *prv_calloc(size_t count, size_t size) {
  return k_calloc(count, size);
}

static void prv_free(void *ptr) {
  k_free(ptr);
}
#else
  #error "CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE or CONFIG_HEAP_MEM_POOL_SIZE must be > 0"
#endif

static void prv_coap_response_cb(const struct coap_client_response_data *data, void *user_data) {
  sMemfaultCoAPContext *ctx = (sMemfaultCoAPContext *)user_data;

  if (data->last_block || data->result_code < 0) {
    ctx->last_result_code = data->result_code;
    ctx->response_received = true;
    k_sem_give(&ctx->response_sem);
  }
}

// Implement nrf_cloud_coap_get_user_options to add Memfault-specific options
// This is called by nRF Cloud CoAP transport when CONFIG_NRF_CLOUD_COAP_MAX_USER_OPTIONS > 0
void nrf_cloud_coap_get_user_options(struct coap_client_option *options, size_t *num_options,
                                     const char *resource, const char *user_data) {
  // Only add options for NRF_CLOUD_COAP_PROXY_RSC resource (Memfault requests)
  if (strcmp(resource, NRF_CLOUD_COAP_PROXY_RSC) != 0) {
    *num_options = 0;
    return;
  }

  const sMemfaultCoAPContext *ctx = (const sMemfaultCoAPContext *)(const void *)user_data;
  if (!ctx || !ctx->proxy_url) {
    *num_options = 0;
    return;
  }

  size_t opt_idx = 0;

  // PROXY_URI option
  size_t proxy_url_len = strlen(ctx->proxy_url);
  if (proxy_url_len > CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE) {
    MEMFAULT_LOG_ERROR("Proxy URL too long: %zu", proxy_url_len);
    *num_options = 0;
    return;
  }
  options[opt_idx].code = COAP_OPTION_PROXY_URI;
  options[opt_idx].len = proxy_url_len;
  memcpy(options[opt_idx].value, ctx->proxy_url, proxy_url_len);
  opt_idx++;

  // Custom project key option
  size_t api_key_len = strlen(g_mflt_http_client_config.api_key);
  if (api_key_len > CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE) {
    MEMFAULT_LOG_ERROR("API key too long: %zu", api_key_len);
    *num_options = 0;
    return;
  }
  options[opt_idx].code = MEMFAULT_NRF_CLOUD_COAP_PROJECT_KEY_OPTION_NO;
  options[opt_idx].len = api_key_len;
  memcpy(options[opt_idx].value, g_mflt_http_client_config.api_key, api_key_len);
  opt_idx++;

  *num_options = opt_idx;
}

static void prv_fota_url_response_cb(const struct coap_client_response_data *data,
                                     void *user_data) {
  sMemfaultCoAPContext *ctx = (sMemfaultCoAPContext *)user_data;

  if (data->last_block || data->result_code < 0) {
    if (data->result_code == COAP_RESPONSE_CODE_CONTENT && data->payload && data->payload_len > 0) {
      // The response is JSON: {"data": {"url": "https://..."}}
      // Extract the URL from the JSON response using manual string parsing
      const char *payload_str = (const char *)data->payload;
      const char *url_start = NULL;
      const char *url_end = NULL;

      const char *url_key = strstr(payload_str, "\"url\"");
      if (url_key) {
        const char *colon = strchr(url_key, ':');
        if (colon) {
          url_start = strchr(colon, '"');
          if (url_start) {
            url_start++;                       // Skip the opening quote
            url_end = strchr(url_start, '"');  // Find the closing quote
          }
        }
      }

      if (url_start && url_end && url_end > url_start) {
        size_t url_len = url_end - url_start;
        ctx->download_url = prv_calloc(1, url_len + 1);
        if (ctx->download_url) {
          memcpy(ctx->download_url, url_start, url_len);
          ctx->download_url[url_len] = '\0';
          MEMFAULT_LOG_DEBUG("Extracted URL from JSON: %s", ctx->download_url);
        } else {
          MEMFAULT_LOG_ERROR("Failed to allocate memory for download URL");
        }
      } else {
        MEMFAULT_LOG_ERROR("Failed to parse URL from JSON response");
      }
    }
    ctx->last_result_code = data->result_code;
    ctx->response_received = true;
    k_sem_give(&ctx->response_sem);
  }
}

static int prv_send_next_msg(sMemfaultCoAPContext *ctx) {
  int rv = 0;
  char *chunk_url = prv_calloc(1, MEMFAULT_PROXY_URL_MAX_LEN);
  if (!chunk_url) {
    return -ENOMEM;
  }

  if (!memfault_http_build_chunk_post_url(chunk_url, MEMFAULT_PROXY_URL_MAX_LEN)) {
    prv_free(chunk_url);
    return -1;
  }

  size_t chunk_size = CONFIG_MEMFAULT_COAP_PACKETIZER_BUFFER_SIZE;
  if (chunk_size > CONFIG_MEMFAULT_COAP_MAX_POST_SIZE) {
    chunk_size = CONFIG_MEMFAULT_COAP_MAX_POST_SIZE;
  }

  uint8_t *chunk_buf = prv_calloc(1, chunk_size);
  if (!chunk_buf) {
    prv_free(chunk_url);
    return -ENOMEM;
  }

  bool data_available = memfault_packetizer_get_chunk(chunk_buf, &chunk_size);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    prv_free(chunk_buf);
    prv_free(chunk_url);
    return 0;
  }

  // Store proxy URL in context for nrf_cloud_coap_get_user_options
  ctx->proxy_url = chunk_url;

  // Reset response state
  ctx->response_received = false;
  ctx->last_result_code = -1;
  k_sem_reset(&ctx->response_sem);

  // Send request using nrf_cloud_coap_post (which will call nrf_cloud_coap_get_user_options)
  MEMFAULT_LOG_DEBUG("Sending CoAP message, size %zu", chunk_size);
  rv = nrf_cloud_coap_post(NRF_CLOUD_COAP_PROXY_RSC, NULL, chunk_buf, chunk_size,
                           COAP_CONTENT_FORMAT_APP_OCTET_STREAM, true, prv_coap_response_cb, ctx);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request: %d", rv);
    memfault_packetizer_abort();
    prv_free(chunk_buf);
    prv_free(chunk_url);
    return -1;
  }

  // Wait for response
  rv = k_sem_take(&ctx->response_sem, K_SECONDS(CONFIG_MEMFAULT_COAP_CLIENT_TIMEOUT_MS / 1000));
  if (rv < 0 || !ctx->response_received) {
    MEMFAULT_LOG_ERROR("Timeout or error waiting for CoAP response: %d", rv);
    prv_free(chunk_buf);
    prv_free(chunk_url);
    return -1;
  }

  if (ctx->last_result_code != COAP_RESPONSE_CODE_CREATED) {
    MEMFAULT_LOG_ERROR("Unexpected CoAP response code: %d", ctx->last_result_code);
    prv_free(chunk_buf);
    prv_free(chunk_url);
    return -1;
  }

  // Count bytes sent
  ctx->bytes_sent += chunk_size;
  prv_free(chunk_buf);
  prv_free(chunk_url);
  return 1;
}

int memfault_zephyr_port_coap_open_socket(sMemfaultCoAPContext *ctx) {
  int rv = 0;

  // If connected, use existing connection. This check is where connection id sharing occurs
  if (!nrf_cloud_coap_is_connected()) {
    MEMFAULT_LOG_DEBUG("nRF Cloud CoAP not connected, connecting...");
    rv = nrf_cloud_coap_connect(NULL);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Failed to connect to nRF Cloud CoAP: %d", rv);
      return -1;
    }
  }

  // Initialize context
  ctx->sock_fd = -1;
  ctx->coap_client = NULL;
  ctx->res = NULL;
  ctx->bytes_sent = 0;
  ctx->download_url = NULL;
  ctx->proxy_url = NULL;
  k_sem_init(&ctx->response_sem, 0, 1);
  ctx->response_received = false;
  ctx->last_result_code = -1;

  return 0;
}

void memfault_zephyr_port_coap_close_socket(sMemfaultCoAPContext *ctx) {
  // Note: We don't manage the socket/client here since they're managed by nRF Cloud CoAP, so we
  // just reset our context
  ctx->sock_fd = -1;
  ctx->coap_client = NULL;
  ctx->res = NULL;
  ctx->proxy_url = NULL;
  ctx->response_received = false;
  ctx->last_result_code = -1;
  // Note: download_url is freed by the caller via memfault_zephyr_port_coap_release_download_url
}

// Conservative CoAP header + options overhead when building chunk proxy requests
#define MEMFAULT_COAP_CHUNK_HEADER_OVERHEAD 256

int memfault_zephyr_port_coap_upload_sdk_data(sMemfaultCoAPContext *ctx) {
  int max_messages_to_send = CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND;

#if defined(CONFIG_MEMFAULT_RAM_BACKED_COREDUMP)
  // Ensure we can send a full coredump if they are stored in RAM, i.e. drainable via the packetizer
  // Payload size per message is capped by MAX_POST_SIZE
  size_t payload_size_per_message = CONFIG_MEMFAULT_COAP_MAX_POST_SIZE;
  if (payload_size_per_message > 0) {
    // Override CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND if we need more messages to send a full
    // coredump
    max_messages_to_send =
      MEMFAULT_MAX(max_messages_to_send,
                   (int)(CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE / payload_size_per_message));
  }
#endif
  bool success = true;
  int rv = -1;

  while (max_messages_to_send-- > 0) {
    rv = prv_send_next_msg(ctx);
    if (rv == 0) {
      // no more messages to send
      break;
    } else if (rv < 0) {
      success = false;
      break;
    }
    success = (rv > 0);
    if (!success) {
      break;
    }
  }

  if ((max_messages_to_send <= 0) && memfault_packetizer_data_available()) {
    MEMFAULT_LOG_WARN(
      "Hit max message limit: " STRINGIFY(CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND));
  }

  return success ? 0 : -1;
}

ssize_t memfault_zephyr_port_coap_post_data_return_size(void) {
  if (!memfault_packetizer_data_available()) {
    return 0;
  }

  sMemfaultCoAPContext ctx = { 0 };
  ctx.sock_fd = -1;

  int rv = memfault_zephyr_port_coap_open_socket(&ctx);
  MEMFAULT_LOG_DEBUG("Opened CoAP socket, rv=%d", rv);

  if (rv == 0) {
    rv = memfault_zephyr_port_coap_upload_sdk_data(&ctx);
    memfault_zephyr_port_coap_close_socket(&ctx);
  }

#if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
  if (rv == 0) {
    memfault_metrics_connectivity_record_memfault_sync_success();
  } else {
    memfault_metrics_connectivity_record_memfault_sync_failure();
  }
#endif

  return (rv == 0) ? (ctx.bytes_sent) : rv;
}

int memfault_zephyr_port_coap_get_download_url(char **download_url) {
  int rv = 0;
  char *latest_url = prv_calloc(1, MEMFAULT_PROXY_URL_MAX_LEN);
  if (!latest_url) {
    return -ENOMEM;
  }

  if (!memfault_http_build_latest_ota_url(latest_url, MEMFAULT_PROXY_URL_MAX_LEN)) {
    prv_free(latest_url);
    return -1;
  }

  sMemfaultCoAPContext ctx = {
    .sock_fd = -1,
    .download_url = NULL,
  };

  rv = memfault_zephyr_port_coap_open_socket(&ctx);
  if (rv != 0) {
    prv_free(latest_url);
    return rv;
  }

  // Store proxy URL in context for nrf_cloud_coap_get_user_options
  ctx.proxy_url = latest_url;

  // Reset response state
  ctx.response_received = false;
  ctx.last_result_code = -1;
  k_sem_reset(&ctx.response_sem);

  // Send request using nrf_cloud_coap_get (which will call nrf_cloud_coap_get_user_options)
  // For GET with no payload: fmt_out can be 0 (ignored since no payload)
  // fmt_in is JSON: {"data": {"url": "https://..."}}
  rv = nrf_cloud_coap_get(NRF_CLOUD_COAP_PROXY_RSC, NULL, NULL, 0, 0, COAP_CONTENT_FORMAT_APP_JSON,
                          true, prv_fota_url_response_cb, &ctx);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request: %d", rv);
    prv_free(latest_url);
    memfault_zephyr_port_coap_close_socket(&ctx);
    return -1;
  }

  rv = k_sem_take(&ctx.response_sem, K_SECONDS(CONFIG_MEMFAULT_COAP_CLIENT_TIMEOUT_MS / 1000));
  if (rv < 0 || !ctx.response_received) {
    MEMFAULT_LOG_ERROR("Timeout or error waiting for CoAP response: %d", rv);
    prv_free(latest_url);
    memfault_zephyr_port_coap_close_socket(&ctx);
    return -1;
  }

  if (ctx.last_result_code == COAP_RESPONSE_CODE_CONTENT && ctx.download_url) {
    *download_url = ctx.download_url;
    rv = 1;  // Update available
  } else if (ctx.last_result_code == COAP_RESPONSE_CODE_NOT_FOUND ||
             ctx.last_result_code == COAP_RESPONSE_CODE_VALID) {
    rv = 0;  // No update available
  } else {
    MEMFAULT_LOG_ERROR("Unexpected CoAP response code: %d", ctx.last_result_code);
    rv = -1;
  }

  prv_free(latest_url);
  memfault_zephyr_port_coap_close_socket(&ctx);
  return rv;
}

int memfault_zephyr_port_coap_release_download_url(char **download_url) {
  prv_free(*download_url);
  *download_url = NULL;
  return 0;
}
