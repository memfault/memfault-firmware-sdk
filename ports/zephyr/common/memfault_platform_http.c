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
#include "memfault/http/http_client.h"
#include "memfault/http/root_certs.h"
#include "memfault/http/utils.h"


#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if !defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
#error "MBEDTLS_SSL_SERVER_NAME_INDICATION must be enabled for Memfault HTTPS. This can be done with CONFIG_MBEDTLS_USER_CONFIG_ENABLE/FILE"
#endif

typedef enum {
  // arbitrarily high base so as not to conflict with id used for other certs in use by the system
  kMemfaultRootCert_Base = 1000,
  kMemfaultRootCert_DstCaX3,
  kMemfaultRootCert_DigicertRootCa,
  kMemfaultRootCert_DigicertRootG2,
} eMemfaultRootCert;

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

static int prv_configure_tls_socket(int sock_fd) {
  sec_tag_t sec_tag_opt[] = { kMemfaultRootCert_DigicertRootG2, kMemfaultRootCert_DigicertRootCa,
                              kMemfaultRootCert_DstCaX3 };
  int rv = setsockopt(sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
  if (rv == -1) {
    return rv;
  }
  const char *host = MEMFAULT_HTTP_GET_API_HOST();
  const size_t host_name_len = strlen(host);
  return setsockopt(sock_fd, SOL_TLS, TLS_HOSTNAME, host, host_name_len + 1);
}

static int prv_configure_socket(struct addrinfo *res, int fd) {
  int rv;
  if (!g_mflt_http_client_config.api_no_tls) {
    rv = prv_configure_tls_socket(fd);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Failed to configure tls, errno=%d", errno);
      return rv;
    }
  }

  return connect(fd, res->ai_addr, res->ai_addrlen);
}

static int prv_open_and_configure_socket(struct addrinfo *res) {
  const int protocol = g_mflt_http_client_config.api_no_tls ? IPPROTO_TCP : IPPROTO_TLS_1_2;
  int fd = socket(res->ai_family, res->ai_socktype, protocol);
  if (fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to open socket, errno=%d", errno);
    return fd;
  }

  int rv = prv_configure_socket(res, fd);
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

static bool prv_wait_for_http_response(int sock_fd) {
  sMemfaultHttpResponseContext ctx = { 0 };
  while (1) {
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

    char buf[8];
    int len = recv(sock_fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (len <= 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }

      MEMFAULT_LOG_ERROR("Receive error: len=%d, errno=%d", len, errno);
      return false;
    }

    bool done = memfault_http_parse_response(&ctx, buf, len);
    if (done) {
      MEMFAULT_LOG_DEBUG("Memfault Message Post Complete: Parse Status %d HTTP Status %d!",
                         (int)ctx.parse_error, ctx.http_status_code);
      MEMFAULT_LOG_DEBUG("Body: %s", ctx.http_body);
      return true;
    }
  }
}

int memfault_zephyr_port_post_data(void) {
    struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res = NULL;
    char port[10] = { 0 };
    snprintf(port, sizeof(port), "%d", MEMFAULT_HTTP_GET_API_PORT());

    const char *host = MEMFAULT_HTTP_GET_API_HOST();
    int rv = getaddrinfo(host, port, &hints, &res);
    if (rv != 0) {
      MEMFAULT_LOG_ERROR("DNS lookup for %s failed: %d", host, rv);
      goto cleanup;
    }

    const int sock_fd = prv_open_and_configure_socket(res);
    if (sock_fd < 0) {
      MEMFAULT_LOG_ERROR("Failed to connect to %s, errno=%d",
                         host, errno);
      rv = -1;
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

cleanup:
    freeaddrinfo(res);
    return rv;
}
