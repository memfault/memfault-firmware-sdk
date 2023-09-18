//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "app_memfault_transport.h"
#include "memfault/esp_port/http_client.h"

void app_memfault_transport_init(void) {}

int app_memfault_transport_send_chunks(void) {
  return memfault_esp_port_http_client_post_data();
}
