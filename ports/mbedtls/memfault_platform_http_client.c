//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Implements platform dependency functions required to utilize Memfault's http client
//! for posting data collected via HTTPS with a stack making using of lwIP & Mbed TLS

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/x509.h"
#include "memfault/components.h"
#include "memfault/ports/mbedtls/http.h"

//! lwIP is by far the most popular port used with Mbed TLS with embedded stacks so by default
//! include a port which implements a minimal mbedtls/net_sockets.h interface.
#ifndef MEMFAULT_PORT_MBEDTLS_USE_LWIP
  #define MEMFAULT_PORT_MBEDTLS_USE_LWIP 1
#endif

#if MEMFAULT_PORT_MBEDTLS_USE_LWIP

  #include "lwip/debug.h"
  #include "lwip/netdb.h"
  #include "lwip/opt.h"
  #include "lwip/sockets.h"
  #include "lwip/stats.h"
  #include "lwip/tcp.h"

void mbedtls_net_init(mbedtls_net_context *ctx) {
  ctx->fd = -1;
}

int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto) {
  struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_flags = AI_PASSIVE,
  };
  struct addrinfo *res = NULL;

  int ret = getaddrinfo(host, port, &hints, &res);
  if ((ret != 0) || (res == NULL)) {
    MEMFAULT_LOG_ERROR("Unable to resolve IP for %s - %d", host, (int)ret);
    return ret;
  }

  ctx->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (ctx->fd < 0) {
    MEMFAULT_LOG_ERROR("Unable to open socket");
    return -1;
  }

  ret = connect(ctx->fd, res->ai_addr, res->ai_addrlen);
  freeaddrinfo(res);

  return ret;
}

int mbedtls_net_send(void *ctx, unsigned char const *buf, size_t len) {
  mbedtls_net_context *net_ctx = (mbedtls_net_context *)ctx;
  return lwip_send(net_ctx->fd, buf, len, 0);
}

int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
  mbedtls_net_context *net_ctx = (mbedtls_net_context *)ctx;
  return lwip_recv(net_ctx->fd, (void *)buf, len, 0);
}

void mbedtls_net_close(mbedtls_net_context *ctx) {
  if (ctx->fd == -1) {
    return;
  }

  close(ctx->fd);
  ctx->fd = -1;
}

void mbedtls_net_free(mbedtls_net_context *ctx) {
  mbedtls_net_close(ctx);
}
#endif /* MEMFAULT_PORT_MBEDTLS_USE_LWIP */

struct MfltHttpClient {
  bool active;

  mbedtls_net_context net_ctx;
  mbedtls_entropy_context entropy;
  mbedtls_hmac_drbg_context hmac_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  uint32_t flags;
  mbedtls_x509_crt cacert;
};

typedef struct MfltHttpResponse {
  // HTTP status code or negative error code
  int status_code;
} sMfltHttpResponse;

static sMfltHttpClient s_client;

static void prv_teardown_mbedtls(sMfltHttpClient *client) {
  mbedtls_net_free(&client->net_ctx);
  mbedtls_x509_crt_free(&client->cacert);
  mbedtls_ssl_free(&client->ssl);
  mbedtls_ssl_config_free(&client->conf);
  mbedtls_hmac_drbg_free(&client->hmac_drbg);
  mbedtls_entropy_free(&client->entropy);
}

//! Generalized SSL/TLS connection setup function
//! @param ssl SSL context to initialize
//! @param conf SSL config to initialize
//! @param hmac_drbg HMAC DRBG context to initialize
//! @param cacert X509 certificate context to initialize
//! @param net_ctx Network context to initialize
//! @param entropy Entropy context to initialize
//! @param host Hostname to connect to
//! @param port Port to connect to (as string)
//! @return 0 on success, -1 on error
static int prv_setup_tls_connection(mbedtls_ssl_context *ssl, mbedtls_ssl_config *conf,
                                    mbedtls_hmac_drbg_context *hmac_drbg, mbedtls_x509_crt *cacert,
                                    mbedtls_net_context *net_ctx, mbedtls_entropy_context *entropy,
                                    const char *host, const char *port) {
  mbedtls_ssl_init(ssl);
  mbedtls_ssl_config_init(conf);
  mbedtls_hmac_drbg_init(hmac_drbg);
  mbedtls_x509_crt_init(cacert);
  mbedtls_net_init(net_ctx);

  mbedtls_entropy_init(entropy);
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

#define HMAC_PERS "memfault"
  int ret = mbedtls_hmac_drbg_seed(hmac_drbg, md_info, mbedtls_entropy_func, entropy,
                                   (const unsigned char *)HMAC_PERS, sizeof(HMAC_PERS) - 1);
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("mbedtls_hmac_drbg_seed returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  ret = mbedtls_x509_crt_parse(cacert, (const unsigned char *)MEMFAULT_ROOT_CERTS_PEM,
                               strlen(MEMFAULT_ROOT_CERTS_PEM));
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("mbedtls_x509_crt_parse returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  ret = mbedtls_net_connect(net_ctx, host, port, MBEDTLS_NET_PROTO_TCP);
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("mbedtls_net_connect returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  ret = mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("mbedtls_ssl_config_defaults returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);
  mbedtls_ssl_conf_rng(conf, mbedtls_hmac_drbg_random, hmac_drbg);

  ret = mbedtls_ssl_setup(ssl, conf);
  if (ret != 0) {
    MEMFAULT_LOG_ERROR("mbedtls_ssl_setup returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  ret = mbedtls_ssl_set_hostname(ssl, host);
  if (ret) {
    MEMFAULT_LOG_ERROR("mbedtls_ssl_set_hostname returned -0x%x", (unsigned int)-ret);
    goto cleanup;
  }

  mbedtls_ssl_set_bio(ssl, net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

  do {
    ret = mbedtls_ssl_handshake(ssl);
    if (ret == 0) {
      break;
    }

    if ((ret == MBEDTLS_ERR_SSL_WANT_READ) || (ret == MBEDTLS_ERR_SSL_WANT_WRITE)) {
      continue;
    } else {
      MEMFAULT_LOG_ERROR("mbedtls_ssl_handshake returned -0x%x", (unsigned int)-ret);
      goto cleanup;
    }
  } while (1);

  return 0;

cleanup:
  mbedtls_net_free(net_ctx);
  mbedtls_x509_crt_free(cacert);
  mbedtls_ssl_free(ssl);
  mbedtls_ssl_config_free(conf);
  mbedtls_hmac_drbg_free(hmac_drbg);
  mbedtls_entropy_free(entropy);
  return -1;
}

sMfltHttpClient *memfault_platform_http_client_create(void) {
  char port[10] = { 0 };

  if (s_client.active) {
    MEMFAULT_LOG_ERROR("Memfault HTTP client already in use");
    return NULL;
  }

  snprintf(port, sizeof(port), "%d", MEMFAULT_HTTP_GET_CHUNKS_API_PORT());
  int ret = prv_setup_tls_connection(&s_client.ssl, &s_client.conf, &s_client.hmac_drbg,
                                     &s_client.cacert, &s_client.net_ctx, &s_client.entropy,
                                     MEMFAULT_HTTP_CHUNKS_API_HOST, port);
  if (ret != 0) {
    return NULL;
  }

  s_client.active = true;
  return &s_client;
}

int memfault_platform_http_client_destroy(sMfltHttpClient *client) {
  if (!client || !client->active) {
    return -1;
  }

  // teardown the connection
  prv_teardown_mbedtls(client);
  s_client.active = false;
  return 0;
}

int memfault_platform_http_client_wait_until_requests_completed(
  MEMFAULT_UNUSED sMfltHttpClient *client, MEMFAULT_UNUSED uint32_t timeout_ms) {
  // No-op because memfault_platform_http_client_post_data() is synchronous
  return 0;
}

static bool prv_try_send(sMfltHttpClient *client, const uint8_t *buf, size_t buf_len) {
  size_t idx = 0;
  while (idx != buf_len) {
    int rv = mbedtls_ssl_write(&client->ssl, (const unsigned char *)buf, buf_len);
    if (rv >= 0) {
      idx += (size_t)rv;
      continue;
    }

    MEMFAULT_LOG_ERROR("Data Send Error: bytes_sent=%d, ret=0x%x", (int)idx, (unsigned int)rv);
    return false;
  }
  return true;
}

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  sMfltHttpClient *client = (sMfltHttpClient *)ctx;
  return prv_try_send(client, data, data_len);
}

static bool prv_read_socket_data(sMfltHttpClient *client, void *buf, size_t *buf_len) {
  int rv = mbedtls_ssl_read(&client->ssl, buf, *buf_len);
  if ((rv == MBEDTLS_ERR_SSL_WANT_READ) || (rv == MBEDTLS_ERR_SSL_WANT_WRITE)) {
    *buf_len = 0;
    return true;
  }

  if ((rv < 0) || (rv == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)) {
    return false;
  }

  *buf_len = (size_t)rv;
  return true;
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
      MEMFAULT_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!", (int)ctx.parse_error,
                         ctx.http_status_code);
      MEMFAULT_LOG_DEBUG("Body: %s", ctx.http_body);
      return ctx.http_status_code;
    }
  }
}

int memfault_platform_http_client_post_data(sMfltHttpClient *client,
                                            MemfaultHttpClientResponseCallback callback,
                                            void *ctx) {
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

  return response.status_code != 202 ? response.status_code : 0;
}

int memfault_platform_http_response_get_status(const sMfltHttpResponse *response,
                                               uint32_t *status_out) {
  MEMFAULT_SDK_ASSERT(response != NULL);

  *status_out = (uint32_t)response->status_code;
  return 0;
}

static int prv_open_ota_tls_connection(sMfltHttpClient *client, const char *url) {
  sMemfaultUriInfo uri_info;
  const size_t url_len = strlen(url);

  if (!memfault_http_parse_uri(url, url_len, &uri_info)) {
    MEMFAULT_LOG_ERROR("Unable to parse url: %s", url);
    return -1;
  }

  // Extract hostname
  char host[uri_info.host_len + 1];
  memcpy(host, uri_info.host, uri_info.host_len);
  host[uri_info.host_len] = '\0';

  char port[10] = { 0 };
  snprintf(port, sizeof(port), "%" PRIu32, uri_info.port);

  int ret =
    prv_setup_tls_connection(&client->ssl, &client->conf, &client->hmac_drbg, &client->cacert,
                             &client->net_ctx, &client->entropy, host, port);
  if (ret != 0) {
    return -1;
  }

  return 0;
}

static bool prv_ota_send_data(const void *data, size_t data_len, void *ctx) {
  sMfltHttpClient *client = (sMfltHttpClient *)ctx;
  size_t idx = 0;
  while (idx != data_len) {
    int rv = mbedtls_ssl_write(&client->ssl, (const unsigned char *)data + idx, data_len - idx);
    if (rv >= 0) {
      idx += (size_t)rv;
      continue;
    }
    MEMFAULT_LOG_ERROR("OTA send error: bytes_sent=%d, ret=0x%x", (int)idx, (unsigned int)rv);
    return false;
  }
  return true;
}

static bool prv_ota_read_data(sMfltHttpClient *client, void *buf, size_t *buf_len) {
  int rv = mbedtls_ssl_read(&client->ssl, buf, *buf_len);
  if ((rv == MBEDTLS_ERR_SSL_WANT_READ) || (rv == MBEDTLS_ERR_SSL_WANT_WRITE)) {
    *buf_len = 0;
    return true;
  }

  if ((rv < 0) || (rv == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)) {
    return false;
  }

  *buf_len = (size_t)rv;
  return true;
}

static bool prv_wait_for_ota_response_header(sMfltHttpClient *client,
                                             sMemfaultHttpResponseContext *http_ctx, void *buf,
                                             size_t *buf_len) {
  const size_t orig_buf_len = *buf_len;
  size_t bytes_read;

  while (1) {
    bytes_read = orig_buf_len;
    if (!prv_ota_read_data(client, buf, &bytes_read)) {
      return false;
    }

    const bool done = memfault_http_parse_response_header(http_ctx, buf, bytes_read);
    if (done) {
      break;
    }
  }

  if (http_ctx->parse_error != kMfltHttpParseStatus_Ok) {
    MEMFAULT_LOG_ERROR("Failed to parse response: Parse Status %d", (int)http_ctx->parse_error);
    return false;
  }

  const size_t message_body_bytes = bytes_read - http_ctx->data_bytes_processed;
  if (message_body_bytes != 0) {
    const void *message_body_bufp = &((uint8_t *)buf)[http_ctx->data_bytes_processed];
    memmove(buf, message_body_bufp, message_body_bytes);
  }
  *buf_len = message_body_bytes;

  if (http_ctx->http_status_code >= 400) {
    MEMFAULT_LOG_ERROR("Unexpected HTTP Status: %d", http_ctx->http_status_code);
    return false;
  }

  return true;
}

static bool prv_install_ota_payload(sMfltHttpClient *client,
                                    const sMemfaultOtaUpdateHandler *handler) {
  sMemfaultHttpResponseContext http_ctx = { 0 };
  size_t buf_len = handler->buf_len;
  if (!prv_wait_for_ota_response_header(client, &http_ctx, handler->buf, &buf_len)) {
    return false;
  }

  sMemfaultOtaInfo ota_info = {
    .size = http_ctx.content_length,
  };

  if (!handler->handle_update_available(&ota_info, handler->user_ctx)) {
    return false;
  }

  if (buf_len != 0 && !handler->handle_data(handler->buf, buf_len, handler->user_ctx)) {
    return false;
  }

  size_t curr_offset = buf_len;
  while (curr_offset != http_ctx.content_length) {
    size_t bytes_read = MEMFAULT_MIN(http_ctx.content_length - curr_offset, handler->buf_len);
    if (!prv_ota_read_data(client, handler->buf, &bytes_read)) {
      return false;
    }

    if (!handler->handle_data(handler->buf, bytes_read, handler->user_ctx)) {
      return false;
    }

    curr_offset += bytes_read;
  }

  return handler->handle_download_complete(handler->user_ctx);
}

static bool prv_fetch_ota_payload(const char *url, const sMemfaultOtaUpdateHandler *handler) {
  sMfltHttpClient client = { 0 };

  if (prv_open_ota_tls_connection(&client, url) != 0) {
    return false;
  }

  const size_t url_len = strlen(url);
  if (!memfault_http_get_ota_payload(prv_ota_send_data, &client, url, url_len)) {
    prv_teardown_mbedtls(&client);
    return false;
  }

  const bool success = prv_install_ota_payload(&client, handler);
  prv_teardown_mbedtls(&client);
  return success;
}

static bool prv_parse_new_ota_payload_url_response(sMfltHttpClient *client,
                                                   char **download_url_out) {
  sMemfaultHttpResponseContext http_ctx = { 0 };

  char working_buf[32];
  size_t working_buf_len = sizeof(working_buf);
  if (!prv_wait_for_ota_response_header(client, &http_ctx, working_buf, &working_buf_len)) {
    return false;
  }

  if (http_ctx.http_status_code != 200) {
    return true;  // up to date!
  }

  const size_t url_len = http_ctx.content_length + 1 /* for '\0' */;
  char *download_url = malloc(url_len);
  if (download_url == NULL) {
    MEMFAULT_LOG_ERROR("Unable to allocate %d bytes for url", (int)url_len);
    return false;
  }

  if (working_buf_len != 0) {
    memcpy(download_url, working_buf, working_buf_len);
  }

  size_t curr_offset = working_buf_len;
  while (curr_offset != http_ctx.content_length) {
    size_t bytes_read = http_ctx.content_length - curr_offset;
    if (!prv_ota_read_data(client, &download_url[curr_offset], &bytes_read)) {
      free(download_url);
      return false;
    }
    curr_offset += bytes_read;
  }

  download_url[http_ctx.content_length] = '\0';
  *download_url_out = download_url;
  return true;
}

static bool prv_check_for_ota_update(char **download_url) {
  char dest_url[512];
  snprintf(dest_url, sizeof(dest_url), "%s://%s", MEMFAULT_HTTP_GET_SCHEME(),
           MEMFAULT_HTTP_GET_DEVICE_API_HOST());

  sMfltHttpClient client = { 0 };
  if (prv_open_ota_tls_connection(&client, dest_url) != 0) {
    return false;
  }

  if (!memfault_http_get_latest_ota_payload_url(prv_ota_send_data, &client)) {
    prv_teardown_mbedtls(&client);
    return false;
  }

  const bool success = prv_parse_new_ota_payload_url_response(&client, download_url);
  prv_teardown_mbedtls(&client);
  return success;
}

int memfault_mbedtls_port_get_download_url(char **download_url) {
  const bool success = prv_check_for_ota_update(download_url);
  if (!success) {
    return -1;  // error
  }

  if (*download_url == NULL) {
    return 0;  // up to date
  }

  return 1;
}

int memfault_mbedtls_port_release_download_url(char **download_url) {
  free(*download_url);
  *download_url = NULL;
  return 0;
}

int memfault_mbedtls_port_ota_update(const sMemfaultOtaUpdateHandler *handler) {
  MEMFAULT_ASSERT((handler != NULL) && (handler->buf != NULL) && (handler->buf_len > 0) &&
                  (handler->handle_update_available != NULL) && (handler->handle_data != NULL) &&
                  (handler->handle_download_complete != NULL));

  char *download_url = NULL;
  bool success = prv_check_for_ota_update(&download_url);
  if (!success) {
    return -1;  // error
  }

  if (download_url == NULL) {
    return 0;  // up to date
  }

  success = prv_fetch_ota_payload(download_url, handler);
  free(download_url);
  return success ? 1 : -1;
}
