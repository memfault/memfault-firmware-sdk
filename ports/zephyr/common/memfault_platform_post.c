//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Selects which backend to use for posting data.

#include "memfault/ports/zephyr/http.h"

#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  // NCS v1.9.2 does not have a recent enough Zephyr version to include this
  // header. This Kconfig option is blocked at CMake stage, so here it is safe
  // to include.
  #include "memfault/nrfconnect_port/coap.h"
#endif

ssize_t memfault_zephyr_port_post_data_return_size(void) {
#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  return memfault_zephyr_port_coap_post_data_return_size();
#else  // default to HTTP transport
  return memfault_zephyr_port_http_post_data_return_size();
#endif
}

int memfault_zephyr_port_post_data(void) {
  ssize_t rv = memfault_zephyr_port_post_data_return_size();
  return (rv >= 0) ? 0 : rv;
}
