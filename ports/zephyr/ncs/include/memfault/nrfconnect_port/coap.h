#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Zephyr specific coap utility for interfacing with Memfault CoAP utilities

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(net/coap.h)

#ifdef __cplusplus
extern "C" {
#endif

//! Context structure used to carry state information about the CoAP connection
typedef struct {
  int sock_fd;
  struct zsock_addrinfo *res;
  size_t bytes_sent;
  uint8_t message_token[COAP_TOKEN_MAX_LEN];
} sMemfaultCoAPContext;

//! Open a socket to the Memfault chunks upload server
//!
//! This function is the simplest way to connect to Memfault. Internally this function combines the
//! required socket operations into a single function call. See the other socket functions for
//! advanced usage.
//!
//! @param ctx If the socket is opened successfully, this will be populated with
//! the connection state for the other COAP functions below
//!
//! @note After use, memfault_zephyr_port_coap_close_socket() must be called
//!  to close the socket and free any associated memory.
//! @note In the case of an error, it is not required to call memfault_zephyr_port_coap_close_socket
//!
//! @return
//!   0 : Success
//! < 0 : Error
int memfault_zephyr_port_coap_open_socket(sMemfaultCoAPContext *ctx);

//! Close a socket previously opened with
//! memfault_zephyr_port_coap_open_socket()
void memfault_zephyr_port_coap_close_socket(sMemfaultCoAPContext *ctx);

//! Identical to memfault_zephyr_port_post_data() but uses the already-opened
//! socket to send data
//!
//! @param ctx Connection context previously opened with
//! memfault_zephyr_port_coap_open_socket()
//!
//! @return 0 on success, -1 on error
int memfault_zephyr_port_coap_upload_sdk_data(sMemfaultCoAPContext *ctx);

//! CoAP specific version of memfault_zephyr_port_post_data_return_size
//!
//! @return negative error code on error, else the size of the data that was
//! sent, in bytes. 0 indicates no data was ready to send (and no data was sent)
ssize_t memfault_zephyr_port_coap_post_data_return_size(void);

#define MEMFAULT_NRF_CLOUD_COAP_PORT 5684
#define MEMFAULT_NRF_CLOUD_COAP_PROJECT_KEY_OPTION_NO 2429

#ifdef __cplusplus
}
#endif
