//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Tests for prv_wait_for_http_response returning the HTTP status code.
//! Specifically verifies that non-200 HTTP responses are treated as failures
//! by memfault_zephyr_port_http_post_chunk (see fix to return int instead of bool).

#include <string.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "memfault/core/platform/device_info.h"
#include "memfault/ports/zephyr/http.h"

// Override the weak default so memfault_http_start_chunk_post can build the request URI.
void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {
  *info = (sMemfaultDeviceInfo){
    .device_serial = "test-device",
    .software_type = "main",
    .software_version = "1.0.0",
    .hardware_version = "dvt",
  };
}

//! Helper that creates a socketpair, writes @p http_response to the remote end, then calls
//! memfault_zephyr_port_http_post_chunk and returns its return value.
//!
//! The HTTP response is pre-written to fds[1] so that prv_wait_for_http_response can read it
//! from fds[0] without blocking. Outgoing request bytes written to fds[0] land in fds[1]'s
//! receive buffer; CONFIG_NET_SOCKETPAIR_BUFFER_SIZE must be large enough to absorb them.
static int prv_post_chunk_with_response(const char *http_response) {
  int fds[2];
  zassert_equal(zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0, "socketpair failed");

  // Stage the response on fds[1]; memfault_zephyr_port_http_post_chunk will read it from fds[0].
  // Loop to handle partial sends - zsock_send() on a stream socket may write fewer bytes than
  // requested.
  const size_t response_len = strlen(http_response);
  size_t total_sent = 0;
  while (total_sent < response_len) {
    const ssize_t sent =
      zsock_send(fds[1], http_response + total_sent, response_len - total_sent, 0);
    zassert_true(sent > 0, "send failed");
    total_sent += (size_t)sent;
  }

  sMemfaultHttpContext ctx = { .sock_fd = fds[0] };
  uint8_t chunk_data[] = { 0x01, 0x02, 0x03 };
  const int rv = memfault_zephyr_port_http_post_chunk(&ctx, chunk_data, sizeof(chunk_data));

  zsock_close(fds[0]);
  zsock_close(fds[1]);
  return rv;
}

ZTEST(memfault_http_response_codes, test_post_chunk_200_response) {
  const int rv = prv_post_chunk_with_response("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
  zassert_equal(rv, 0, "Expected 0 for 200 response, got %d", rv);
}

ZTEST(memfault_http_response_codes, test_post_chunk_non_200_response) {
  const int rv =
    prv_post_chunk_with_response("HTTP/1.1 429 Too Many Requests\r\nContent-Length: 0\r\n\r\n");
  zassert_equal(rv, -3, "Expected -3 for 429 response, got %d", rv);
}

ZTEST_SUITE(memfault_http_response_codes, NULL, NULL, NULL, NULL, NULL);
