//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!

#include "memfault_zephyr_http.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <init.h>
#include <kernel.h>
#include <misc/printk.h>
#include <net/socket.h>
#include <net/tls_credentials.h>
#include <zephyr.h>

#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/http/http_client.h"
#include "memfault/http/root_certs.h"
#include "memfault/http/utils.h"
#include "memfault/panics/assert.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if !defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
#error "MBEDTLS_SSL_SERVER_NAME_INDICATION must be enabled for Memfault HTTPS. This can be done with CONFIG_MBEDTLS_USER_CONFIG_ENABLE/FILE"
#endif

#define MEMFAULT_NUM_CERTS_REGISTERED 5

#if !defined(CONFIG_TLS_MAX_CREDENTIALS_NUMBER) || (CONFIG_TLS_MAX_CREDENTIALS_NUMBER < MEMFAULT_NUM_CERTS_REGISTERED)
# pragma message("ERROR: CONFIG_TLS_MAX_CREDENTIALS_NUMBER must be >= "MEMFAULT_EXPAND_AND_QUOTE(MEMFAULT_NUM_CERTS_REGISTERED))
# error "Update CONFIG_TLS_MAX_CREDENTIALS_NUMBER in prj.conf"
#endif

#if !defined(CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS) || (CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS < MEMFAULT_NUM_CERTS_REGISTERED)
# pragma message("ERROR: CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS must be >= "MEMFAULT_EXPAND_AND_QUOTE(MEMFAULT_NUM_CERTS_REGISTERED))
# error "Update CONFIG_NET_SOCKETS_TLS_MAX_CREDENTIALS in prj.conf"
#endif

typedef enum {
  // arbitrarily high base so as not to conflict with id used for other certs in use by the system
  kMemfaultRootCert_Base = 1000,
  kMemfaultRootCert_DstCaX3,
  kMemfaultRootCert_DigicertRootCa,
  kMemfaultRootCert_DigicertRootG2,
  kMemfaultRootCert_CyberTrustRoot,
  kMemfaultRootCert_AmazonRootCa1,
  // Must be last, used to track number of root certs in use
  kkMemfaultRootCert_MaxIndex,
} eMemfaultRootCert;

MEMFAULT_STATIC_ASSERT(
    (kkMemfaultRootCert_MaxIndex - (kMemfaultRootCert_Base + 1)) == MEMFAULT_NUM_CERTS_REGISTERED,
    "MEMFAULT_NUM_CERTS_REGISTERED define out of sync");


static int prv_memfault_http_install_certs(struct device *dev) {

  int rv = tls_credential_add(kMemfaultRootCert_DstCaX3, TLS_CREDENTIAL_CA_CERTIFICATE,
                              g_memfault_cert_dst_ca_x3, g_memfault_cert_dst_ca_x3_len);
  if (rv != 0) {
    goto error;
  }

  rv = tls_credential_add(kMemfaultRootCert_DigicertRootCa, TLS_CREDENTIAL_CA_CERTIFICATE,
                          g_memfault_cert_digicert_global_root_ca,
                          g_memfault_cert_digicert_global_root_ca_len);
  if (rv != 0) {
    goto error;
  }

  rv = tls_credential_add(kMemfaultRootCert_DigicertRootG2, TLS_CREDENTIAL_CA_CERTIFICATE,
                          g_memfault_cert_digicert_global_root_g2,
                          g_memfault_cert_digicert_global_root_g2_len);
  if (rv != 0) {
    goto error;
  }

  rv = tls_credential_add(kMemfaultRootCert_CyberTrustRoot, TLS_CREDENTIAL_CA_CERTIFICATE,
                          g_memfault_cert_baltimore_cybertrust_root,
                          g_memfault_cert_baltimore_cybertrust_root_len);
  if (rv != 0) {
    goto error;
  }

  rv = tls_credential_add(kMemfaultRootCert_AmazonRootCa1, TLS_CREDENTIAL_CA_CERTIFICATE,
                          g_memfault_cert_amazon_root_ca1,
                          g_memfault_cert_amazon_root_ca1_len);
  if (rv != 0) {
    goto error;
  }

  return 0; // success!
error:
  MEMFAULT_LOG_ERROR("Cert init failed, rv=%d", rv);
  return rv;
}

SYS_INIT(prv_memfault_http_install_certs, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  int fd = *(int *)ctx;
  int rv = send(fd, data, data_len, 0);
  return (rv == data_len);
}

static int prv_configure_tls_socket(int sock_fd, const char *host) {
  sec_tag_t sec_tag_opt[] = { kMemfaultRootCert_DigicertRootG2, kMemfaultRootCert_DigicertRootCa,
                              kMemfaultRootCert_DstCaX3,  kMemfaultRootCert_CyberTrustRoot,
                              kMemfaultRootCert_AmazonRootCa1 };
  int rv = setsockopt(sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
  if (rv != 0) {
    return rv;
  }

  const size_t host_name_len = strlen(host);
  return setsockopt(sock_fd, SOL_TLS, TLS_HOSTNAME, host, host_name_len + 1);
}

static int prv_configure_socket(struct addrinfo *res, int fd, const char *host) {
  int rv;
  if (!g_mflt_http_client_config.disable_tls) {
    rv = prv_configure_tls_socket(fd, host);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Failed to configure tls, errno=%d", errno);
      return rv;
    }
  }

  return connect(fd, res->ai_addr, res->ai_addrlen);
}

static int prv_open_and_configure_socket(struct addrinfo *res, const char *host) {
  const int protocol = g_mflt_http_client_config.disable_tls ? IPPROTO_TCP : IPPROTO_TLS_1_2;

  int fd = socket(res->ai_family, res->ai_socktype, protocol);
  if (fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to open socket, errno=%d", errno);
    return fd;
  }

  int rv = prv_configure_socket(res, fd, host);
  if (rv < 0) {
    close(fd);
    return rv;
  }
  return fd;
}

static bool prv_try_send(int sock_fd, const uint8_t *buf, size_t buf_len) {
  size_t idx = 0;
  while (idx != buf_len) {
    int rv = send(sock_fd, &buf[idx], buf_len - idx, 0);
    if (rv > 0) {
      idx += rv;
      continue;
    }
    if (rv <= 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      MEMFAULT_LOG_ERROR("Data Send Error: len=%d, errno=%d", (int)buf_len, errno);
      return false;
    }
  }
  return true;
}

static int prv_open_socket(struct addrinfo **res, const char *host, int port_num) {
  struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };

  char port[10] = { 0 };
  snprintf(port, sizeof(port), "%d", port_num);

  int rv = getaddrinfo(host, port, &hints, res);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("DNS lookup for %s failed: %d", host, rv);
    return -1;
  }

  const int sock_fd = prv_open_and_configure_socket(*res, host);
  if (sock_fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to connect to %s, errno=%d",
                       host, errno);
    return -1;
  }
  return sock_fd;
}

static bool prv_send_next_msg(int sock) {
  const sPacketizerConfig cfg = {
    // let a single msg span many "memfault_packetizer_get_next" calls
    .enable_multi_packet_chunk = true,
  };

  // will be populated with size of entire message queued for sending
  sPacketizerMetadata metadata;
  const bool data_available = memfault_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    return false;
  }

  memfault_http_start_chunk_post(prv_send_data, &sock, metadata.single_chunk_message_length);

  while (1) {
    uint8_t buf[128];
    size_t buf_len = sizeof(buf);
    eMemfaultPacketizerStatus status = memfault_packetizer_get_next(buf, &buf_len);
    if (status == kMemfaultPacketizerStatus_NoMoreData) {
      break;
    }

    if (!prv_try_send(sock, buf, buf_len)) {
      // unexpected failure, abort in-flight transaction
      memfault_packetizer_abort();
      return false;
    }

    if (status == kMemfaultPacketizerStatus_EndOfChunk) {
      break;
    }
  }

  // message sent, await response
  return true;
}

static int prv_read_socket_data(int sock_fd, void *buf, size_t *buf_len) {
  struct pollfd poll_fd = {
    .fd = sock_fd,
    .events = POLLIN,
  };
  const int timeout_ms = 5000;
  int rv = poll(&poll_fd, 1, timeout_ms);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Timeout awaiting response: errno=%d", errno);
    return false;
  }

  const int len = recv(sock_fd, buf, *buf_len, MSG_DONTWAIT);
  if (len <= 0) {
    if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
      *buf_len = 0;
      return true;
    }

    MEMFAULT_LOG_ERROR("Receive error: len=%d, errno=%d", len, errno);
    return false;
  }
  *buf_len = len;
  return true;
}

static bool prv_wait_for_http_response(int sock_fd) {
  sMemfaultHttpResponseContext ctx = { 0 };
  while (1) {
    char buf[32]; // arbitrary receive buf small
    size_t bytes_read = sizeof(buf);
    if (!prv_read_socket_data(sock_fd, buf, &bytes_read)) {
      return false;
    }

    bool done = memfault_http_parse_response(&ctx, buf, bytes_read);
    if (done) {
      MEMFAULT_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!",
                         (int)ctx.parse_error, ctx.http_status_code);
      MEMFAULT_LOG_DEBUG("Body: %s", ctx.http_body);
      return true;
    }
  }
}

static bool prv_wait_for_http_response_header(int sock_fd, sMemfaultHttpResponseContext *ctx,
                                              void *buf, size_t *buf_len) {
  const size_t orig_buf_len = *buf_len;
  size_t bytes_read;

  while (1) {
    bytes_read = orig_buf_len;
    if (!prv_read_socket_data(sock_fd, buf, &bytes_read)) {
      return false;
    }

    const bool done = memfault_http_parse_response_header(ctx, buf, bytes_read);
    if (done) {
      break;
    }
  }

  if (ctx->parse_error != kMfltHttpParseStatus_Ok) {
    MEMFAULT_LOG_ERROR("Failed to parse response: Parse Status %d", (int)ctx->parse_error);
    return false;
  }

  // Move unprocessed message-body bytes to the beginning of the working buf
  // and update buf_len with the number of read bytes
  const size_t message_body_bytes = bytes_read - ctx->data_bytes_processed;
  if (message_body_bytes != 0) {
    const void *message_body_bufp = &((uint8_t *)buf)[ctx->data_bytes_processed];
    memmove(buf, message_body_bufp, message_body_bytes);
  }
  *buf_len = message_body_bytes;

  if (ctx->http_status_code >= 400) {
    MEMFAULT_LOG_ERROR("Unexpected HTTP Status: %d", ctx->http_status_code);
    // A future improvement here would be to dump the message-body on error
    // if it is text or application/json
    return false;
  }

  return true;
}

static bool prv_install_ota_payload(int sock_fd,
                                    const sMemfaultOtaUpdateHandler *handler) {
  sMemfaultHttpResponseContext ctx = { 0 };
  size_t buf_len = handler->buf_len;
  if (!prv_wait_for_http_response_header(sock_fd, &ctx, handler->buf, &buf_len)) {
    return false;
  }

  sMemfaultOtaInfo ota_info = {
    .size = ctx.content_length,
  };

  if (!handler->handle_update_available(&ota_info, handler->user_ctx)) {
    return false;
  }

  if (buf_len != 0 && !handler->handle_data(handler->buf, buf_len, handler->user_ctx)) {
    return false;
  }

  size_t curr_offset = buf_len;
  while (curr_offset != ctx.content_length) {
    size_t bytes_read = MEMFAULT_MIN(ctx.content_length - curr_offset, handler->buf_len);
    if (!prv_read_socket_data(sock_fd, handler->buf, &bytes_read)) {
      return false;
    }

    if (!handler->handle_data(handler->buf, bytes_read, handler->user_ctx)) {
      return false;
    }

    curr_offset += bytes_read;
  }

  return handler->handle_download_complete(handler->user_ctx);
}

static bool prv_fetch_ota_payload(const char *url,
                                  const sMemfaultOtaUpdateHandler *handler) {
  bool success = false;

  sMemfaultUriInfo uri_info;
  const size_t url_len = strlen(url);

  if (!memfault_http_parse_uri(url, url_len, &uri_info)) {
    MEMFAULT_LOG_ERROR("Unable to parse url: %s", url);
    return false;
  }

  // create nul terminated host string from parsed url
  char host[uri_info.host_len + 1];
  memcpy(host, uri_info.host, uri_info.host_len);
  host[uri_info.host_len] = '\0';

  // create the connection
  struct addrinfo *res = NULL;
  int sock_fd = prv_open_socket(&res, host, uri_info.port);
  if (sock_fd < 0) {
    goto cleanup;
  }

  if (!memfault_http_get_ota_payload(prv_send_data, &sock_fd, url, url_len)) {
    goto cleanup;
  }

  success = prv_install_ota_payload(sock_fd, handler);
cleanup:
  if (sock_fd >= 0) {
    close(sock_fd);
  }

  if (res != NULL) {
    freeaddrinfo(res);
  }
  return success;
}

static bool prv_parse_new_ota_payload_url_response(int sock_fd, char **download_url_out) {
  sMemfaultHttpResponseContext ctx = { 0 };

  char working_buf[32];
  size_t working_buf_len = sizeof(working_buf);
  if (!prv_wait_for_http_response_header(sock_fd, &ctx, working_buf, &working_buf_len)) {
    return false;
  }

  if (ctx.http_status_code != 200) {
    return true; // up to date!
  }

  const size_t url_len = ctx.content_length + 1 /* for '\0' */;
  char *download_url = calloc(1, url_len);
  if (download_url == NULL) {
    MEMFAULT_LOG_ERROR("Unable to allocate %d bytes for url", (int)url_len);
    return false;
  }

  // copy any parts of the message-body we already received into the storage holding
  // the download url
  if (working_buf_len != 0) {
    memcpy(download_url, working_buf, working_buf_len);
  }

  size_t curr_offset = working_buf_len;
  while (curr_offset != ctx.content_length) {
    size_t bytes_read = ctx.content_length - curr_offset;
    if (!prv_read_socket_data(sock_fd, &download_url[curr_offset], &bytes_read)) {
      goto error;
    }
    curr_offset += bytes_read;
  }

  *download_url_out = download_url;
  return true;
error:
  free(download_url);
  return false;
}

//! Checks to see if a new OTA Update is available
//!
//! @return true on success, false otherwise. On success, the download_url will be populated with
//! the link to the new ota payload iff a new update is available.
static bool prv_check_for_ota_update(char **download_url) {
  bool success = false;

  const char *host = MEMFAULT_HTTP_GET_DEVICE_API_HOST();
  const int port = MEMFAULT_HTTP_GET_DEVICE_API_PORT();

  struct addrinfo *res = NULL;
  int sock_fd = prv_open_socket(&res, host, port);
  if (sock_fd < 0) {
    goto cleanup;
  }

  if (!memfault_http_get_latest_ota_payload_url(prv_send_data, &sock_fd)) {
    goto cleanup;
  }

  // We successfully sent the http request for a latest ota payload parse the response
  success = prv_parse_new_ota_payload_url_response(sock_fd, download_url);
cleanup:
  if (sock_fd >= 0) {
    close(sock_fd);
  }

  freeaddrinfo(res);
  return success;
}

int memfault_zephyr_port_ota_update(const sMemfaultOtaUpdateHandler *handler) {
  MEMFAULT_ASSERT(
      (handler != NULL) &&
      (handler->buf != NULL) && (handler->buf > 0) &&
      (handler->handle_update_available != NULL) &&
      (handler->handle_data != NULL) &&
      (handler->handle_download_complete != NULL));

  char *download_url = NULL;
  bool success = prv_check_for_ota_update(&download_url);
  if (!success) {
    return -1; // error
  }

  if (download_url == NULL) {
    return 0; // up to date
  }

  success = prv_fetch_ota_payload(download_url, handler);
  free(download_url);
  return success ? 1 : -1;
}

int memfault_zephyr_port_post_data(void) {
  int rv = -1;

  const char *host = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();
  const int port =  MEMFAULT_HTTP_GET_CHUNKS_API_PORT();

  struct addrinfo *res = NULL;
  const int sock_fd = prv_open_socket(&res, host, port);
  if (sock_fd < 0) {
    goto cleanup;
  }

  int max_messages_to_send = 5;
  while (max_messages_to_send-- > 0) {
    if (!prv_send_next_msg(sock_fd)) {
      break;
    }
    if (!prv_wait_for_http_response(sock_fd)) {
      break;
    }
  }

  close(sock_fd);

  // if we got here, everything succeeded!
  rv = 0;
cleanup:
  freeaddrinfo(res);
  return rv;
}
