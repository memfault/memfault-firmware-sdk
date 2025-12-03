//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/coap.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/socket.h)
#include <date_time.h>
#include <memfault/metrics/connectivity.h>
#include <modem/modem_jwt.h>
#include <nrf_modem_at.h>
#include <zephyr/random/random.h>

#include "memfault/core/data_packetizer.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/platform/device_info.h"
#include "memfault/http/http_client.h"
#include "memfault/http/utils.h"
#include "memfault/nrfconnect_port/coap.h"
#include "memfault/ports/zephyr/http.h"

extern sMfltHttpClientConfig g_mflt_http_client_config;

#if defined(CONFIG_MEMFAULT_NCS_PROJECT_KEY)
  #undef CONFIG_MEMFAULT_PROJECT_KEY
  #define CONFIG_MEMFAULT_PROJECT_KEY CONFIG_MEMFAULT_NCS_PROJECT_KEY
#endif  // defined(CONFIG_MEMFAULT_NCS_PROJECT_KEY)

static int prv_generate_jwt(char *jwt_buf, size_t jwt_buf_sz, int sec_tag) {
  int err = 0;
  int retry_count = 0;

  // wait until modem has valid time
  const int max_retries = 60;  // 60 seconds max wait
  while (nrf_modem_at_cmd(jwt_buf, jwt_buf_sz, "AT%%CCLK?")) {
    if (++retry_count >= max_retries) {
      return -ETIMEDOUT;
    }
    k_sleep(K_SECONDS(1));
  }

  struct jwt_data jwt = {
    .audience = NULL,
    .key = JWT_KEY_TYPE_CLIENT_PRIV,
    .alg = JWT_ALG_TYPE_ES256,
    .jwt_buf = jwt_buf,
    .jwt_sz = jwt_buf_sz,
    .exp_delta_s = 60 * 60,  // one hour is plenty
    .sec_tag = sec_tag,
    .subject = NULL,
  };
  err = modem_jwt_generate(&jwt);
  if (err) {
    MEMFAULT_LOG_ERROR("Failed to generate JWT, error: %d", err);
  }
  return err;
}

static const uint8_t coap_auth_request_template[] = {
  0x48, 0x02,                                      // CoAP POST
  0xFF, 0xFF,                                      // Message ID
  0x90, 0x4A, 0x50, 0x61, 0xBF, 0x40, 0x33, 0x40,  // Token
  0xB4, 0x61, 0x75, 0x74, 0x68,                    // URI: "auth"
  0x03, 0x6A, 0x77, 0x74,                          // URI "jwt"
  0x10,                                            // Content-Type Text
  0xFF,                                            // Payload-Marker
};
#define JWT_BUF_SZ 600
#define COAP_AUTH_REQ_HEADER_SIZE (sizeof(coap_auth_request_template))
#define COAP_AUTH_REQ_BUF_SIZE (JWT_BUF_SZ + COAP_AUTH_REQ_HEADER_SIZE)
#define COAP_AUTH_REQ_MID_OFFSET 2
#define COAP_AUTH_REQ_TOKEN_OFFSET 4
#define COAP_REQ_WAIT_TIME_MS 300

static int auth_socket(int socket) {
  int32_t err = 0;
  struct coap_packet reply;
  uint8_t token[COAP_TOKEN_MAX_LEN];
  uint8_t *packet_buf = k_malloc(COAP_AUTH_REQ_BUF_SIZE);
  uint8_t response_token[COAP_TOKEN_MAX_LEN];
  uint16_t mid = sys_cpu_to_be16(coap_next_id());
  size_t request_size;

  if (!packet_buf) {
    MEMFAULT_LOG_ERROR("Failed to allocate memory for CoAP auth request buffer");
    err = -ENOMEM;
    goto end;
  }

  // Make new random token
  sys_rand_get(token, COAP_TOKEN_MAX_LEN);

  // Fill in CoAP header
  memcpy(packet_buf, coap_auth_request_template, COAP_AUTH_REQ_HEADER_SIZE);
  memcpy(packet_buf + COAP_AUTH_REQ_MID_OFFSET, &mid, sizeof(mid));
  memcpy(packet_buf + COAP_AUTH_REQ_TOKEN_OFFSET, token, COAP_TOKEN_MAX_LEN);

  // Generate JWT
  err = prv_generate_jwt((char *)packet_buf + COAP_AUTH_REQ_HEADER_SIZE, JWT_BUF_SZ,
                         CONFIG_MEMFAULT_NRF_CLOUD_SEC_TAG);
  if (err) {
    MEMFAULT_LOG_ERROR("Error generating JWT with modem: %d", err);
    goto end;
  }

  request_size =
    COAP_AUTH_REQ_HEADER_SIZE + strnlen((char *)packet_buf + COAP_AUTH_REQ_HEADER_SIZE, JWT_BUF_SZ);

  // Send the request
  MEMFAULT_LOG_DEBUG("Sending CoAP auth request, size %zu", request_size);
  err = zsock_send(socket, packet_buf, request_size, 0);
  if (err < 0) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request, errno %d", errno);
    goto end;
  }

  for (size_t i = 0; i < 10; ++i) {
    k_sleep(K_MSEC(COAP_REQ_WAIT_TIME_MS));
    // Poll for response
    err = zsock_recv(socket, packet_buf, COAP_AUTH_REQ_BUF_SIZE, ZSOCK_MSG_DONTWAIT);
    if (err < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      } else {
        err = -errno;
        MEMFAULT_LOG_ERROR("Error receiving response: %d", err);
        goto end;
      }
    }
    MEMFAULT_LOG_DEBUG("Received CoAP auth response, size %d", err);
    // Parse response
    err = coap_packet_parse(&reply, packet_buf, err, NULL, 0);
    if (err < 0) {
      MEMFAULT_LOG_ERROR("Error parsing response: %d", err);
      continue;
    }
    // Match token
    coap_header_get_token(&reply, response_token);
    if (memcmp(response_token, token, COAP_TOKEN_MAX_LEN) != 0) {
      continue;
    }
    // Check response code
    if (coap_header_get_code(&reply) != COAP_RESPONSE_CODE_CREATED) {
      MEMFAULT_LOG_ERROR("Error in response: %d", coap_header_get_code(&reply));
      continue;
    } else {
      err = 0;
      goto end;
    }
  }
  err = -ETIMEDOUT;

end:
  k_free(packet_buf);
  return err;
}

static int prv_getaddrinfo(struct zsock_addrinfo **res, const char *host, int port_num) {
  struct zsock_addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_DGRAM,
  };

  char port[10] = { 0 };
  snprintf(port, sizeof(port), "%d", port_num);

  int rv = zsock_getaddrinfo(host, port, &hints, res);
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

static int prv_create_socket(struct zsock_addrinfo **res, const char *host, int port_num) {
  int rv = prv_getaddrinfo(res, host, port_num);
  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to get address info, error: %d", rv);
    return rv;
  }

  int fd = zsock_socket((*res)->ai_family, (*res)->ai_socktype, IPPROTO_DTLS_1_2);
  if (fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to open socket, errno=%d", errno);
  }

  return fd;
}

static int prv_configure_dtls_socket(int sock_fd, const char *host) {
  const int sec_tag_opt[] = { CONFIG_MEMFAULT_NRF_CLOUD_SEC_TAG };
  int rv = zsock_setsockopt(sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
  int verify = TLS_PEER_VERIFY_REQUIRED;

  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to setup sec tag, err %d", errno);
    return rv;
  }

  rv = zsock_setsockopt(sock_fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to setup peer verification, err %d", errno);
    return rv;
  }

  uint32_t dtls_cid = TLS_DTLS_CID_ENABLED;

  rv = zsock_setsockopt(sock_fd, SOL_TLS, TLS_DTLS_CID, &dtls_cid, sizeof(dtls_cid));
  if (rv) {
    MEMFAULT_LOG_ERROR("Failed to setup CID, err %d", errno);
    return rv;
  }

  return zsock_setsockopt(sock_fd, SOL_TLS, TLS_HOSTNAME, host, strlen(host) + 1);
}

static int prv_configure_socket(int fd, const char *host) {
  int rv = 0;

  rv = prv_configure_dtls_socket(fd, host);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to configure tls w/ host, errno=%d", errno);
  }

  return rv;
}

static int prv_connect_socket(int fd, struct zsock_addrinfo *res) {
  int rv = zsock_connect(fd, res->ai_addr, res->ai_addrlen);

  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to connect socket, errno=%d", errno);
    zsock_close(fd);
  }

  return rv;
}

static int prv_open_socket(struct zsock_addrinfo **res, const char *host, int port_num) {
  const int sock_fd = prv_create_socket(res, host, port_num);
  if (sock_fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to create socket, errno=%d", sock_fd);
    return sock_fd;
  }

  int rv = prv_configure_socket(sock_fd, host);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to configure socket, errno=%d", rv);
    zsock_close(sock_fd);
    return rv;
  }

  rv = prv_connect_socket(sock_fd, *res);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to connect socket, errno=%d", rv);
    zsock_close(sock_fd);
    return rv;
  }

  rv = auth_socket(sock_fd);
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to auth socket, errno=%d", rv);
    zsock_close(sock_fd);
    return rv;
  }
  return sock_fd;
}

static int memfault_coap_make_header(sMemfaultCoAPContext *ctx, uint8_t *buf, size_t buf_len) {
  const char *uri_path = "proxy";
  const char *proxy_uri = "https://chunks.memfault.com/api/v0/chunks/%s";
  sMemfaultDeviceInfo info = { 0 };

  memfault_http_get_device_info(&info);

  const size_t proxy_uri_max_len = strlen(proxy_uri) - 2 + strlen(info.device_serial) + 1;
  uint8_t *proxy_uri_buf = k_malloc(proxy_uri_max_len);

  if (!proxy_uri_buf) {
    return -ENOMEM;
  }

  snprintf(proxy_uri_buf, proxy_uri_max_len, proxy_uri, info.device_serial);

  int rv = 0;
  struct coap_packet request;
  memcpy(ctx->message_token, coap_next_token(), sizeof(ctx->message_token));

  rv = coap_packet_init(&request, buf, buf_len, 1, COAP_TYPE_CON, COAP_TOKEN_MAX_LEN,
                        ctx->message_token, COAP_METHOD_POST, coap_next_id());
  if (rv) {
    goto exit;
  }
  rv = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
  if (rv) {
    goto exit;
  }
  rv = coap_packet_append_option(&request, COAP_OPTION_PROXY_URI, proxy_uri_buf,
                                 strlen(proxy_uri_buf));
  if (rv) {
    goto exit;
  }
  rv = coap_packet_append_option(&request, MEMFAULT_NRF_CLOUD_COAP_PROJECT_KEY_OPTION_NO,
                                 CONFIG_MEMFAULT_PROJECT_KEY, strlen(CONFIG_MEMFAULT_PROJECT_KEY));
  if (rv) {
    goto exit;
  }
  rv = coap_packet_append_payload_marker(&request);
  if (rv) {
    goto exit;
  }

  rv = request.offset;

exit:
  k_free(proxy_uri_buf);
  return rv;
}

static int prv_poll_socket(int sock_fd, int events) {
  struct zsock_pollfd poll_fd = {
    .fd = sock_fd,
    .events = events,
  };
  const int timeout_ms = CONFIG_MEMFAULT_COAP_CLIENT_TIMEOUT_MS;
  int rv = zsock_poll(&poll_fd, 1, timeout_ms);
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
    int rv = prv_poll_socket(sock_fd, ZSOCK_POLLOUT);
    if (rv <= 0) {
      MEMFAULT_LOG_ERROR("Socket not ready for send: errno=%d", errno);
      return false;
    }

    rv = zsock_send(sock_fd, &buf[idx], buf_len - idx, ZSOCK_MSG_DONTWAIT);
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

//! Returns:
//! 0  - no more data to send
//! 1  - data sent, awaiting response
//! -1 - error
static int prv_send_next_msg(sMemfaultCoAPContext *ctx) {
  int sock = ctx->sock_fd;
  uint8_t *buf = k_malloc(CONFIG_MEMFAULT_COAP_PACKETIZER_BUFFER_SIZE);
  size_t buf_len = CONFIG_MEMFAULT_COAP_PACKETIZER_BUFFER_SIZE;
  size_t chunk_size = 0;
  int header_len = 0;
  int rv = 0;

  if (!buf) {
    MEMFAULT_LOG_ERROR("Failed to allocate buffer for reading data");
    return -1;
  }

  header_len = memfault_coap_make_header(ctx, buf, buf_len);
  if (header_len < 0) {
    MEMFAULT_LOG_ERROR("Failed to make coap header, err: %d", header_len);
    rv = -1;
    goto exit;
  }
  chunk_size = buf_len - header_len;

  bool data_available = memfault_packetizer_get_chunk(buf + header_len, &chunk_size);
  if (!data_available) {
    MEMFAULT_LOG_DEBUG("No more data to send");
    rv = 0;
    goto exit;
  }

  buf_len = header_len + chunk_size;

  MEMFAULT_LOG_DEBUG("Sending CoAP message, size %zu, fd %d", buf_len, sock);

  if (!prv_try_send(sock, buf, buf_len)) {
    MEMFAULT_LOG_ERROR("Failed to send CoAP request, errno %d", errno);
    memfault_packetizer_abort();
    rv = -1;
    goto exit;
  }

  // count bytes sent
  ctx->bytes_sent += buf_len;
  rv = 1;

  // message sent, await response
exit:
  k_free(buf);
  return rv;
}

int memfault_zephyr_port_coap_open_socket(sMemfaultCoAPContext *ctx) {
  const char *host = CONFIG_MEMFAULT_NRF_CLOUD_HOST_NAME;
  const int port = MEMFAULT_NRF_CLOUD_COAP_PORT;

  memfault_zephyr_port_coap_close_socket(ctx);

  MEMFAULT_LOG_DEBUG("Opening socket to %s:%d", host, port);

  ctx->sock_fd = prv_open_socket(&(ctx->res), host, port);

  if (ctx->sock_fd < 0) {
    MEMFAULT_LOG_ERROR("Failed to open socket: %d", ctx->sock_fd);
    memfault_zephyr_port_coap_close_socket(ctx);
    return -1;
  }

  MEMFAULT_LOG_DEBUG("Socket fd=%d", ctx->sock_fd);

  return 0;
}

void memfault_zephyr_port_coap_close_socket(sMemfaultCoAPContext *ctx) {
  if (ctx->sock_fd >= 0) {
    zsock_close(ctx->sock_fd);
  }
  ctx->sock_fd = -1;

  if (ctx->res != NULL) {
    freeaddrinfo(ctx->res);
    ctx->res = NULL;
  }
}

static int prv_wait_for_coap_response(sMemfaultCoAPContext *ctx) {
  int rv = -1;
  uint8_t packet_buf[32];  // empirically 17 bytes
  struct coap_packet reply;
  uint8_t response_token[COAP_TOKEN_MAX_LEN];

  while (true) {
    rv = prv_poll_socket(ctx->sock_fd, ZSOCK_POLLIN);
    if (rv <= 0) {
      return -1;
    }
    rv = zsock_recv(ctx->sock_fd, packet_buf, sizeof(packet_buf), ZSOCK_MSG_DONTWAIT);
    if (rv < 0) {
      rv = -errno;
      MEMFAULT_LOG_ERROR("Error receiving response: %d", rv);
      return rv;
    }
    MEMFAULT_LOG_DEBUG("Received CoAP response, size %d", rv);
    // Parse response
    rv = coap_packet_parse(&reply, packet_buf, rv, NULL, 0);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Error parsing response: %d", rv);
      return rv;
    }
    // Match token
    coap_header_get_token(&reply, response_token);
    if (memcmp(response_token, ctx->message_token, COAP_TOKEN_MAX_LEN) != 0) {
      continue;
    }
    // Check response code
    if (coap_header_get_code(&reply) != COAP_RESPONSE_CODE_CREATED) {
      MEMFAULT_LOG_ERROR("Unexpected response code: %d", coap_header_get_code(&reply));
      return -1;
    } else {
      return 0;
    }
  }
}

int memfault_zephyr_port_coap_upload_sdk_data(sMemfaultCoAPContext *ctx) {
  int max_messages_to_send = CONFIG_MEMFAULT_HTTP_MAX_MESSAGES_TO_SEND;

#if CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE && CONFIG_MEMFAULT_RAM_BACKED_COREDUMP
  // The largest data type we will send is a coredump. If CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE
  // is being used, make sure we issue enough HTTP POSTS such that an entire coredump will be sent.
  max_messages_to_send =
    MEMFAULT_MAX(max_messages_to_send,
                 CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE / CONFIG_MEMFAULT_HTTP_MAX_POST_SIZE);
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
    success = !prv_wait_for_coap_response(ctx);
    if (!success) {
      break;
    }
  }

  if ((max_messages_to_send <= 0) && memfault_packetizer_data_available()) {
    MEMFAULT_LOG_WARN(
      "Hit max message limit: " STRINGIFY(CONFIG_MEMFAULT_HTTP_MAX_MESSAGES_TO_SEND));
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

#if defined(CONFIG_MEMFAULT_METRICS_SYNC_SUCCESS)
  if (rv == 0) {
    memfault_metrics_connectivity_record_memfault_sync_success();
  } else {
    memfault_metrics_connectivity_record_memfault_sync_failure();
  }
#endif

  return (rv == 0) ? (ctx.bytes_sent) : rv;
}
