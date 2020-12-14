//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!

#include "memfault/nrfconnect_port/http.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <init.h>
#include <kernel.h>
#include <modem/modem_key_mgmt.h>
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

typedef enum {
  // arbitrarily high base so as not to conflict with id used for other certs in use by the system
  kMemfaultRootCert_Base = 1000,
  kMemfaultRootCert_DstCaX3,
  kMemfaultRootCert_DigicertRootCa,
  kMemfaultRootCert_DigicertRootG2,
  kMemfaultRootCert_CyberTrustRoot,
  kMemfaultRootCert_AmazonRootCa1,
  // Must be last, used to track number of root certs in use
  kMemfaultRootCert_MaxIndex,
} eMemfaultRootCert;

static bool prv_install_cert(eMemfaultRootCert cert_id) {
  bool exists;
  uint8_t unused;

  const char *cert;
  size_t cert_len;

  switch (cert_id) {
    case kMemfaultRootCert_DstCaX3:
      cert = MEMFAULT_ROOT_CERTS_DST_ROOT_CA_X3;
      cert_len = strlen(MEMFAULT_ROOT_CERTS_DST_ROOT_CA_X3);
      break;
    case kMemfaultRootCert_DigicertRootCa:
      cert = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA;
      cert_len = strlen(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA);
      break;
    case kMemfaultRootCert_DigicertRootG2:
      cert = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2;
      cert_len = strlen(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2);
      break;
    case kMemfaultRootCert_CyberTrustRoot:
      cert = MEMFAULT_ROOT_CERTS_BALTIMORE_CYBERTRUST_ROOT;
      cert_len = strlen(MEMFAULT_ROOT_CERTS_BALTIMORE_CYBERTRUST_ROOT);
      break;
    case kMemfaultRootCert_AmazonRootCa1:
      cert = MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1;
      cert_len = strlen(MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1);
      break;

    default:
      MEMFAULT_LOG_ERROR("Unknown cert id: %d", (int)cert_id);
      return -1;
  }

  int err = modem_key_mgmt_exists(cert_id,
                              MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                              &exists, &unused);
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to install cert %d, rv=%d\n", cert_id, err);
    return err;
  }

  if (exists) {
    return 0;
  }

  MEMFAULT_LOG_INFO("Installing Root CA %d", cert_id);
  err = modem_key_mgmt_write(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                             cert, cert_len);
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to provision certificate, err %d\n", err);
  }
  return err;
}

int memfault_nrfconnect_port_install_root_certs(void) {
  for (eMemfaultRootCert cert_id = kMemfaultRootCert_DstCaX3; cert_id <  kMemfaultRootCert_MaxIndex;
       cert_id ++) {
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

static int prv_configure_tls_socket(int sock_fd, const char *host) {
  const sec_tag_t sec_tag_opt[] = {
    kMemfaultRootCert_DigicertRootG2, kMemfaultRootCert_DigicertRootCa,
    kMemfaultRootCert_DstCaX3,  kMemfaultRootCert_CyberTrustRoot,
    kMemfaultRootCert_AmazonRootCa1 };
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
    printk("Failed to setup peer verification, err %d\n", errno);
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
    // We don't expect any response that needs to be parsed so
    // just use an arbitrarily small receive buffer
    char buf[32];
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

int memfault_nrfconnect_port_post_data(void) {
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
