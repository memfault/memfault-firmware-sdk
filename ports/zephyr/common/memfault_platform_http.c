//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

// clang-format off
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include MEMFAULT_ZEPHYR_INCLUDE(init.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/tls_credentials.h)

#include "memfault/core/compiler.h"
#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/http/http_client.h"
#include "memfault/http/root_certs.h"
#include "memfault/http/utils.h"
#include "memfault/metrics/connectivity.h"
#include "memfault/panics/assert.h"
#include "memfault/ports/zephyr/http.h"
#include "memfault/ports/zephyr/root_cert_storage.h"
#include "memfault/ports/zephyr/deprecated_root_cert.h"

#if MEMFAULT_ZEPHYR_VERSION_GT_STRICT(3, 6)

//! Zephyr 3.7.0 deprecated NET_SOCKETS_POSIX_NAMES, and requires POSIX_API to be enabled instead.
//! Confirm it's set.
#if !defined(CONFIG_POSIX_API)
#error "CONFIG_POSIX_API must be enabled"
#endif

//! Zephyr 3.7.0 removed default enabling of hash algorithms needed for CA certificate parsing. Confirm the one we need is set.
#if defined(CONFIG_MBEDTLS_BUILTIN) && !defined(CONFIG_MBEDTLS_SHA1)
#error "CONFIG_MBEDTLS_SHA1 must be enabled"
#endif
#endif

#if defined(CONFIG_POSIX_API)
  #include MEMFAULT_ZEPHYR_INCLUDE(posix/netdb.h)
  #include MEMFAULT_ZEPHYR_INCLUDE(posix/poll.h)
  #include MEMFAULT_ZEPHYR_INCLUDE(posix/sys/socket.h)
  #include MEMFAULT_ZEPHYR_INCLUDE(posix/unistd.h)
#else
  #include MEMFAULT_ZEPHYR_INCLUDE(net/socket.h)
#endif
// clang-format on

#if defined(CONFIG_MEMFAULT_HTTP_USES_MBEDTLS)

// Sanity check that SNI extension is enabled when using Mbed TLS since as of 2.4 Zephyr doesn't
// enable it by default

  #if !defined(MBEDTLS_CONFIG_FILE)
    #if MEMFAULT_ZEPHYR_VERSION_GT(3, 5)
      #include <mbedtls/build_info.h>
    #else
      #include "mbedtls/config.h"
    #endif
  #else
    #include MBEDTLS_CONFIG_FILE
  #endif

  #if !defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
    #error \
      "MBEDTLS_SSL_SERVER_NAME_INDICATION must be enabled for Memfault HTTPS. This can be done with CONFIG_MBEDTLS_USER_CONFIG_ENABLE/FILE"
  #endif

#endif /* CONFIG_MEMFAULT_HTTP_USES_MBEDTLS */

// Timeout for the socket file descriptor to become available for I/0
#define POLL_TIMEOUT_MS 5000

// The nRF-Connect SDK has an implementation of this structure
#if !defined(CONFIG_MEMFAULT_NCS_PROJECT_KEY)
MEMFAULT_STATIC_ASSERT(
  sizeof(CONFIG_MEMFAULT_PROJECT_KEY) > 1,
  "Memfault Project Key not configured. Please visit https://mflt.io/project-key "
  "and add CONFIG_MEMFAULT_PROJECT_KEY=\"YOUR_KEY\" to prj.conf");

sMfltHttpClientConfig g_mflt_http_client_config = {
  .api_key = CONFIG_MEMFAULT_PROJECT_KEY,
};
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

// Select either PEM or DER format to install to certificate storage.
// clang-format off
#if defined(CONFIG_MEMFAULT_TLS_CERTS_USE_DER)
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_ptr g_memfault_cert_digicert_global_root_ca
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_len g_memfault_cert_digicert_global_root_ca_len
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_ptr g_memfault_cert_digicert_global_root_g2
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_len g_memfault_cert_digicert_global_root_g2_len
  #define MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_ptr g_memfault_cert_amazon_root_ca1
  #define MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_len g_memfault_cert_amazon_root_ca1_len
#elif defined(CONFIG_MEMFAULT_TLS_CERTS_USE_PEM)
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_ptr MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_len sizeof(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA)
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_ptr MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2
  #define MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_len sizeof(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2)
  #define MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_ptr MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1
  #define MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_len sizeof(MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1)
#else
#error "Must choose a cert format. Check CONFIG_MEMFAULT_TLS_CERTS_FORMAT for options and their dependencies."
#endif
// clang-format on

static bool prv_install_cert(eMemfaultRootCert cert_id) {
  const char *cert;
  size_t cert_len;

  switch (cert_id) {
    case kMemfaultRootCert_DigicertRootCa:
      cert = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_ptr;
      cert_len = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA_len;
      break;
    case kMemfaultRootCert_DigicertRootG2:
      cert = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_ptr;
      cert_len = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2_len;
      break;
    case kMemfaultRootCert_AmazonRootCa1:
      cert = MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_ptr;
      cert_len = MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1_len;
      break;

    default:
      MEMFAULT_LOG_ERROR("Unknown cert id: %d", (int)cert_id);
      return -1;
  }

  return memfault_root_cert_storage_add(cert_id, cert, cert_len);
}

int memfault_zephyr_port_install_root_certs(void) {
  for (eMemfaultDeprecatedRootCert cert_id = kMemfaultDeprecatedRootCert_DuplicateAmazonRootCa1;
       cert_id < kMemfaultDeprecatedRootCert_MaxIndex; cert_id++) {
    memfault_root_cert_storage_remove(cert_id);
  }

  for (eMemfaultRootCert cert_id = kMemfaultRootCert_DigicertRootG2;
       cert_id < kMemfaultRootCert_MaxIndex; cert_id++) {
    const int rv = prv_install_cert(cert_id);
    if (rv != 0) {
      return rv;
    }
  }

  return 0;
}

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  int fd = *(int *)ctx;
  int rv = send(fd, data, data_len, 0);
  return (rv == data_len);
}

static int prv_getaddrinfo(struct zsock_addrinfo **res, const char *host, int port_num) {
  struct zsock_addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };

  char port[10] = { 0 };
  snprintf(port, sizeof(port), "%d", port_num);

  int rv = getaddrinfo(host, port, &hints, res);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("DNS lookup for %s failed: %d", host, rv);
  } else {
    struct sockaddr_in *addr = net_sin((*res)->ai_addr);

    MEMFAULT_LOG_DEBUG("DNS lookup for %s = %d.%d.%d.%d", host, addr->sin_addr.s4_addr[0],
                       addr->sin_addr.s4_addr[1], addr->sin_addr.s4_addr[2],
                       addr->sin_addr.s4_addr[3]);
  }

  return rv;
}

static int prv_create_socket(struct addrinfo **res, const char *host, int port_num) {
  const int protocol = g_mflt_http_client_config.disable_tls ? IPPROTO_TCP : IPPROTO_TLS_1_2;

  int rv = prv_getaddrinfo(res, host, port_num);
  if (rv) {
    return rv;
  }

  int fd = socket((*res)->ai_family, (*res)->ai_socktype, protocol);
  if (fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to open socket, errno=%d", errno);
  }

  return fd;
}

static int prv_configure_tls_socket(int sock_fd, const char *host) {
  const sec_tag_t sec_tag_opt[] = { kMemfaultRootCert_DigicertRootG2,
                                    kMemfaultRootCert_AmazonRootCa1,
                                    kMemfaultRootCert_DigicertRootCa };
  int rv = setsockopt(sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
  if (rv != 0) {
    return rv;
  }

  /* Set up TLS peer verification */
  enum {
    NONE = 0,
    OPTIONAL = 1,
    REQUIRED = 2,
  };

  int verify = REQUIRED;
  rv = setsockopt(sock_fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to setup peer verification, err %d\n", errno);
    return rv;
  }

  // Set TLS cert parse + copy to optional, which will allow us to use either PEM or DER formatted
  // certs. This feature was added in Zephyr v3.0.0. Not all socket operation implementations
  // support this socket option, so gate on CONFIG_MEMFAULT_TLS_CERTS_USE_DER to ensure dependencies
  // are met. Parsing PEM does not require this socket option.
#if defined(CONFIG_MEMFAULT_TLS_CERTS_USE_DER) && defined(TLS_CERT_NOCOPY_OPTIONAL)
  const int nocopy = TLS_CERT_NOCOPY_OPTIONAL;
  rv = setsockopt(sock_fd, SOL_TLS, TLS_CERT_NOCOPY, &nocopy, sizeof(nocopy));
  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to set tls nocopy, err %d\n", errno);
    return rv;
  }
#endif

  const size_t host_name_len = strlen(host);
  return setsockopt(sock_fd, SOL_TLS, TLS_HOSTNAME, host, host_name_len + 1);
}

static int prv_configure_socket(int fd, const char *host) {
  int rv = 0;
  if (!g_mflt_http_client_config.disable_tls) {
    rv = prv_configure_tls_socket(fd, host);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Failed to configure tls w/ host, errno=%d", errno);
    }
  }

  return rv;
}

static int prv_connect_socket(int fd, struct addrinfo *res) {
  int rv = connect(fd, res->ai_addr, res->ai_addrlen);

  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to connect socket, errno=%d", errno);
    close(fd);
  }

  return rv;
}

static int prv_poll_socket(int sock_fd, int events) {
  struct pollfd poll_fd = {
    .fd = sock_fd,
    .events = events,
  };
  const int timeout_ms = POLL_TIMEOUT_MS;
  int rv = poll(&poll_fd, 1, timeout_ms);
  if (rv == 0) {
    MEMFAULT_LOG_ERROR("Timeout waiting for socket event(s): event(s)=%d, errno=%d", events, errno);
  }
  return rv;
}

static bool prv_try_send(int sock_fd, const uint8_t *buf, size_t buf_len) {
  size_t idx = 0;
  while (idx != buf_len) {
    // Wait for socket to become available within a timeout in case the socket is busy processing
    // other tx data. This will prevent busy looping in this loop (since the send call is
    // non-blocking), therefore allowing other threads to run.
    int rv = prv_poll_socket(sock_fd, POLLOUT);
    if (rv <= 0) {
      return false;
    }

    rv = send(sock_fd, &buf[idx], buf_len - idx, MSG_DONTWAIT);
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

static int prv_open_socket(struct zsock_addrinfo **res, const char *host, int port_num) {
  const int sock_fd = prv_create_socket(res, host, port_num);
  if (sock_fd < 0) {
    return -1;
  }

  int rv = prv_configure_socket(sock_fd, host);
  if (rv < 0) {
    close(sock_fd);
    return -1;
  }

  rv = prv_connect_socket(sock_fd, *res);
  if (rv < 0) {
    return -1;
  }
  return sock_fd;
}

//! Returns:
//! 0  - no more data to send
//! 1  - data sent, awaiting response
//! -1 - error
static int prv_send_next_msg(sMemfaultHttpContext *ctx) {
  int sock = ctx->sock_fd;

#if CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE
  uint8_t buf[CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE];
  size_t buf_len = sizeof(buf);

  bool data_available = memfault_packetizer_get_chunk(buf, &buf_len);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    return 0;  // no more data to send
  }

  memfault_http_start_chunk_post(prv_send_data, &sock, buf_len);

  if (!prv_try_send(sock, buf, buf_len)) {
    // unexpected failure, abort in-flight transaction
    memfault_packetizer_abort();
    return -1;
  }

  // count bytes sent
  ctx->bytes_sent += buf_len;

  // message sent, await response
  return 1;

#else
  const sPacketizerConfig cfg = {
    // let a single msg span many "memfault_packetizer_get_next" calls
    .enable_multi_packet_chunk = true,
  };

  // will be populated with size of entire message queued for sending
  sPacketizerMetadata metadata;
  const bool data_available = memfault_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    return 0;
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
      return -1;
    }

    // count bytes sent
    ctx->bytes_sent += buf_len;

    if (status == kMemfaultPacketizerStatus_EndOfChunk) {
      break;
    }
  }

  // message sent, await response
  return 1;
#endif /* CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE */
}

static int prv_read_socket_data(int sock_fd, void *buf, size_t *buf_len) {
  int rv = prv_poll_socket(sock_fd, POLLIN);
  if (rv <= 0) {
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
    // We don't expect any response that needs to be parsed so
    // just use an arbitrarily small receive buffer
    char buf[32];
    size_t bytes_read = sizeof(buf);
    if (!prv_read_socket_data(sock_fd, buf, &bytes_read)) {
      return false;
    }

    bool done = memfault_http_parse_response(&ctx, buf, bytes_read);
    if (done) {
      MEMFAULT_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!", (int)ctx.parse_error,
                         ctx.http_status_code);
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

static bool prv_install_ota_payload(int sock_fd, const sMemfaultOtaUpdateHandler *handler) {
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

static bool prv_fetch_ota_payload(const char *url, const sMemfaultOtaUpdateHandler *handler) {
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
    return true;  // up to date!
  }

  const size_t url_len = ctx.content_length + 1 /* for '\0' */;
  char *download_url = prv_calloc(1, url_len);
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
  prv_free(download_url);
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

  if (res) {
    freeaddrinfo(res);
  }
  return success;
}

int memfault_zephyr_port_get_download_url(char **download_url) {
  const bool success = prv_check_for_ota_update(download_url);
  if (!success) {
    return -1;  // error
  }

  if (*download_url == NULL) {
    return 0;  // up to date
  }

  return 1;
}

int memfault_zephyr_port_release_download_url(char **download_url) {
  prv_free(*download_url);
  *download_url = NULL;
  return 0;
}

int memfault_zephyr_port_ota_update(const sMemfaultOtaUpdateHandler *handler) {
  MEMFAULT_ASSERT((handler != NULL) && (handler->buf != NULL) && (handler->buf > 0) &&
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
  prv_free(download_url);
  return success ? 1 : -1;
}

ssize_t memfault_zephyr_port_post_data_return_size(void) {
  sMemfaultHttpContext ctx = { 0 };
  ctx.sock_fd = -1;

  int rv = memfault_zephyr_port_http_open_socket(&ctx);

  if (rv == 0) {
    rv = memfault_zephyr_port_http_upload_sdk_data(&ctx);
  }

  if (rv == 0) {
    memfault_zephyr_port_http_close_socket(&ctx);
  }

#if defined(CONFIG_MEMFAULT_METRICS_SYNC_SUCCESS)
  if (rv == 0) {
    memfault_metrics_connectivity_record_memfault_sync_success();
  } else {
    memfault_metrics_connectivity_record_memfault_sync_failure();
  }
#endif

  return (rv == 0) ? (ctx.bytes_sent) : rv;
}

int memfault_zephyr_port_post_data(void) {
  ssize_t rv = memfault_zephyr_port_post_data_return_size();
  return (rv >= 0) ? 0 : rv;
}

int memfault_zephyr_port_http_open_socket(sMemfaultHttpContext *ctx) {
  const char *host = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();
  const int port = MEMFAULT_HTTP_GET_CHUNKS_API_PORT();

  memfault_zephyr_port_http_close_socket(ctx);

  MEMFAULT_LOG_DEBUG("Opening socket to %s:%d", host, port);

  ctx->sock_fd = prv_open_socket(&(ctx->res), host, port);

  if (ctx->sock_fd < 0) {
    memfault_zephyr_port_http_close_socket(ctx);
    return -1;
  }

  return 0;
}

int memfault_zephyr_port_http_create_socket(sMemfaultHttpContext *ctx) {
  const char *host = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();
  const int port = MEMFAULT_HTTP_GET_CHUNKS_API_PORT();

  memfault_zephyr_port_http_close_socket(ctx);

  ctx->sock_fd = prv_create_socket(&(ctx->res), host, port);

  if (ctx->sock_fd < 0) {
    memfault_zephyr_port_http_close_socket(ctx);
    return -1;
  }

  return 0;
}

int memfault_zephyr_port_http_configure_socket(sMemfaultHttpContext *ctx) {
  const char *host = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();

  int rv = prv_configure_socket(ctx->sock_fd, host);
  if (rv < 0) {
    memfault_zephyr_port_http_close_socket(ctx);
  }

  return rv;
}

int memfault_zephyr_port_http_connect_socket(sMemfaultHttpContext *ctx) {
  int rv = prv_connect_socket(ctx->sock_fd, ctx->res);
  if (rv < 0) {
    memfault_zephyr_port_http_close_socket(ctx);
  }

  return rv;
}

void memfault_zephyr_port_http_close_socket(sMemfaultHttpContext *ctx) {
  if (ctx->sock_fd >= 0) {
    close(ctx->sock_fd);
  }
  ctx->sock_fd = -1;

  if (ctx->res != NULL) {
    freeaddrinfo(ctx->res);
    ctx->res = NULL;
  }
}

bool memfault_zephyr_port_http_is_connected(sMemfaultHttpContext *ctx) {
  return ctx->sock_fd >= 0;
}

int memfault_zephyr_port_http_upload_sdk_data(sMemfaultHttpContext *ctx) {
  int max_messages_to_send = 5;
#if CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE && CONFIG_MEMFAULT_RAM_BACKED_COREDUMP
  // The largest data type we will send is a coredump. If CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE
  // is being used, make sure we issue enough HTTP POSTS such that an entire coredump will be sent.
  max_messages_to_send =
    MEMFAULT_MAX(max_messages_to_send,
                 CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE / CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE);
#endif
  bool success = true;

  while (max_messages_to_send-- > 0) {
    int rv = prv_send_next_msg(ctx);
    if (rv == 0) {
      // no more messages to send
      break;
    } else if (rv < 0) {
      success = false;
      break;
    }
    success = prv_wait_for_http_response(ctx->sock_fd);
    if (!success) {
      break;
    }
  }
  return success ? 0 : -1;
}

int memfault_zephyr_port_http_post_chunk(sMemfaultHttpContext *ctx, void *p_data, size_t data_len) {
  if (!memfault_zephyr_port_http_is_connected(ctx)) {
    return -1;
  }

  memfault_http_start_chunk_post(prv_send_data, &(ctx->sock_fd), data_len);

  if (!prv_try_send(ctx->sock_fd, p_data, data_len)) {
    return -2;
  }

  if (!prv_wait_for_http_response(ctx->sock_fd)) {
    return -3;
  }

  return 0;
}
