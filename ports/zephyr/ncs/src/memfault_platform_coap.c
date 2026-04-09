//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include <errno.h>
#include <limits.h>
#include <stdio.h>

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/coap.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/coap_client.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/socket.h)
#include MEMFAULT_ZEPHYR_INCLUDE(random/random.h)

#include <date_time.h>
#include <modem/modem_jwt.h>
#include <net/nrf_cloud_coap.h>
#include <nrf_cloud_coap_transport.h>
#include <nrf_modem_at.h>

#include "memfault/components.h"
#include "memfault/nrfconnect_port/coap.h"
#include "memfault/ports/ncs/version.h"
// clang-format on

// Restrict to versions with the nrf_cloud_coap_get_user_options() API, used to optionally inject
// the Memfault project key via custom CoAP option 2429 for the native /chunks endpoint.
#if !MEMFAULT_NCS_VERSION_GT(3, 2)
  #error \
    "The Memfault CoAP port currently requires support added after nRF Connect SDK version 3.2.1. Please upgrade to nRF Connect SDK > 3.2.1"
#endif

//! Default buffer size for proxy URLS. This may need to be increased by the user if there are
//! particularly long device properties (serial, hardware version, software version, or type)
#ifndef MEMFAULT_PROXY_URL_MAX_LEN
  #define MEMFAULT_PROXY_URL_MAX_LEN (200)
#endif

#if CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND == 0
  #error \
    "CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND must not be 0. Use -1 for unlimited or a positive value to cap uploads."
#endif

// Each Memfault chunk maps 1:1 to one CoAP block (no blockwise transfer per chunk).
// The payload is bounded by BLOCK_SIZE with ~100 B reserved for CoAP options overhead
// (Block1, project key, etc.).
#define MEMFAULT_COAP_CHUNK_SIZE \
  (CONFIG_COAP_CLIENT_BLOCK_SIZE + CONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE - 100)

// Used by memfault_zephyr_port_coap_post_data_return_size() and
// memfault_zephyr_port_coap_get_download_url(), i.e. when the user does not provide their own
// context. File scope here ensures context can be referenced by async callbacks
static sMemfaultCoAPContext s_async_ctx;

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

  // Caller timed out and returned, abort the operation
  if (ctx->last_result_code == -ETIMEDOUT) {
    return;
  }

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
  // For the native /chunks endpoint, optionally inject the Memfault project key (CoAP option 2429).
  // If no project key is configured, the server uses its auto-configured mapping.
  if (strcmp(resource, "chunks") == 0) {
    size_t api_key_len =
      g_mflt_http_client_config.api_key ? strlen(g_mflt_http_client_config.api_key) : 0;
    if (api_key_len == 0) {
      // No project key configured; rely on server-side auto-forwarding
      *num_options = 0;
      return;
    }
    if (api_key_len > CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE) {
      MEMFAULT_LOG_ERROR("API key too long: %zu", api_key_len);
      *num_options = 0;
      return;
    }
    options[0].code = MEMFAULT_NRF_CLOUD_COAP_PROJECT_KEY_OPTION_NO;
    options[0].len = api_key_len;
    memcpy(options[0].value, g_mflt_http_client_config.api_key, api_key_len);
    *num_options = 1;
    return;
  }

  // For the proxy resource (used by FOTA URL lookup), add PROXY_URI and project key options
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
  size_t api_key_len =
    g_mflt_http_client_config.api_key ? strlen(g_mflt_http_client_config.api_key) : 0;
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

  // Caller timed out and returned, abort URL parsing
  if (ctx->last_result_code == -ETIMEDOUT) {
    return;
  }

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

  size_t chunk_size = CONFIG_MEMFAULT_COAP_PACKETIZER_BUFFER_SIZE;
  if (chunk_size > MEMFAULT_COAP_CHUNK_SIZE) {
    chunk_size = MEMFAULT_COAP_CHUNK_SIZE;
  }

  uint8_t *chunk_buf = prv_calloc(1, chunk_size);
  if (!chunk_buf) {
    return -ENOMEM;
  }

  bool data_available = memfault_packetizer_get_chunk(chunk_buf, &chunk_size);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    prv_free(chunk_buf);
    return 0;
  }

  // Reset response state
  ctx->response_received = false;
  ctx->last_result_code = -1;
  k_sem_reset(&ctx->response_sem);

  // Send chunk data to the native /chunks endpoint. The project key (CoAP option 2429) is
  // injected optionally via nrf_cloud_coap_get_user_options if configured.
  MEMFAULT_LOG_DEBUG("Sending CoAP message, size %zu", chunk_size);
  rv = nrf_cloud_coap_post("chunks", NULL, chunk_buf, chunk_size,
                           COAP_CONTENT_FORMAT_APP_OCTET_STREAM, true, prv_coap_response_cb, ctx);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request: %d", rv);
    memfault_packetizer_abort();
    prv_free(chunk_buf);
    return -1;
  }

  // Wait for response
  rv = k_sem_take(&ctx->response_sem, K_SECONDS(CONFIG_MEMFAULT_COAP_CLIENT_TIMEOUT_MS / 1000));
  if (rv < 0 || !ctx->response_received) {
    MEMFAULT_LOG_ERROR("Timeout or error waiting for CoAP response: %d", rv);
    ctx->last_result_code = -ETIMEDOUT;  // Signal async callback to abort
    prv_free(chunk_buf);
    return -1;
  }

  if (ctx->last_result_code != COAP_RESPONSE_CODE_CREATED) {
    MEMFAULT_LOG_ERROR("Unexpected CoAP response code: %d", ctx->last_result_code);
    prv_free(chunk_buf);
    return -1;
  }

  // Count bytes sent
  ctx->bytes_sent += chunk_size;
  prv_free(chunk_buf);
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
  *ctx = (sMemfaultCoAPContext){
    .sock_fd = -1,
    .coap_client = NULL,
    .res = NULL,
    .download_url = NULL,
    .proxy_url = NULL,
    .response_received = false,
    .last_result_code = -1,
  };

  k_sem_init(&ctx->response_sem, 0, 1);
  return 0;
}

void memfault_zephyr_port_coap_close_socket(sMemfaultCoAPContext *ctx) {
  // Note: We don't manage the socket/client here since they're managed by nRF Cloud CoAP, so we
  // just reset our context
  *ctx = (sMemfaultCoAPContext){
    .sock_fd = -1,
    .coap_client = NULL,
    .res = NULL,
    .proxy_url = NULL,
    .response_received = false,
    .last_result_code = -1,
  };
  // Note: download_url is freed by the caller via memfault_zephyr_port_coap_release_download_url
}

int memfault_zephyr_port_coap_upload_sdk_data(sMemfaultCoAPContext *ctx) {
#if CONFIG_MEMFAULT_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE
  // Dedicated workqueue: drain everything — no risk of blocking other work.
  int max_messages_to_send = INT_MAX;
#else
  // System workqueue: cap message count to avoid blocking other work.
  // -1 means unlimited; normalize to INT_MAX so the loop condition works correctly.
  int max_messages_to_send = (CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND < 0) ?
                               INT_MAX :
                               CONFIG_MEMFAULT_COAP_MAX_MESSAGES_TO_SEND;
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

  sMemfaultCoAPContext *ctx = &s_async_ctx;

  int rv = memfault_zephyr_port_coap_open_socket(ctx);
  MEMFAULT_LOG_DEBUG("Opened CoAP socket, rv=%d", rv);

  size_t bytes_sent = 0;
  if (rv == 0) {
    rv = memfault_zephyr_port_coap_upload_sdk_data(ctx);
    bytes_sent = ctx->bytes_sent;
    if (ctx->last_result_code != -ETIMEDOUT) {
      memfault_zephyr_port_coap_close_socket(ctx);
    }
  }

#if defined(CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS)
  if (rv == 0) {
    memfault_metrics_connectivity_record_memfault_sync_success();
  } else {
    memfault_metrics_connectivity_record_memfault_sync_failure();
  }
#endif

  return (rv == 0) ? (ssize_t)bytes_sent : (ssize_t)rv;
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

  sMemfaultCoAPContext *ctx = &s_async_ctx;

  rv = memfault_zephyr_port_coap_open_socket(ctx);
  if (rv != 0) {
    prv_free(latest_url);
    return rv;
  }

  // Store proxy URL in context for nrf_cloud_coap_get_user_options
  ctx->proxy_url = latest_url;

  // Reset response state
  ctx->response_received = false;
  ctx->last_result_code = -1;
  k_sem_reset(&ctx->response_sem);

  // Send request using nrf_cloud_coap_get (which will call nrf_cloud_coap_get_user_options)
  // For GET with no payload: fmt_out can be 0 (ignored since no payload)
  // fmt_in is JSON: {"data": {"url": "https://..."}}
  rv = nrf_cloud_coap_get(NRF_CLOUD_COAP_PROXY_RSC, NULL, NULL, 0, 0, COAP_CONTENT_FORMAT_APP_JSON,
                          true, prv_fota_url_response_cb, ctx);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request: %d", rv);
    prv_free(latest_url);
    memfault_zephyr_port_coap_close_socket(ctx);
    return -1;
  }

  rv = k_sem_take(&ctx->response_sem, K_MSEC(CONFIG_MEMFAULT_COAP_CLIENT_TIMEOUT_MS));
  if (rv < 0 || !ctx->response_received) {
    MEMFAULT_LOG_ERROR("Timeout or error waiting for CoAP response: %d", rv);
    ctx->last_result_code = -ETIMEDOUT;  // Signal async callback not to touch context
    prv_free(latest_url);
    return -1;
  }

  if (ctx->last_result_code == COAP_RESPONSE_CODE_CONTENT && ctx->download_url) {
    *download_url = ctx->download_url;
    rv = 1;  // Update available
  } else if (ctx->last_result_code == COAP_RESPONSE_CODE_NOT_FOUND ||
             ctx->last_result_code == COAP_RESPONSE_CODE_VALID) {
    rv = 0;  // No update available
  } else {
    MEMFAULT_LOG_ERROR("Unexpected CoAP response code: %d", ctx->last_result_code);
    rv = -1;
  }

  prv_free(latest_url);
  memfault_zephyr_port_coap_close_socket(ctx);
  return rv;
}

int memfault_zephyr_port_coap_release_download_url(char **download_url) {
  prv_free(*download_url);
  *download_url = NULL;
  return 0;
}
