#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Zephyr specific http utility for interfacing with Memfault HTTP utilities

#include <stdbool.h>
#include <stddef.h>

#include "memfault/ports/zephyr/http.h"

#ifdef __cplusplus
extern "C" {
#endif

//! memfault_nrfconnect_port_* names are now deprecated.
//!
//! This is a temporary change to map to the appropriate name but will be removed in
//! future releases

#define memfault_nrfconnect_port_install_root_certs memfault_zephyr_port_install_root_certs
#define memfault_nrfconnect_port_post_data memfault_zephyr_port_post_data
#define memfault_nrfconnect_port_ota_update memfault_zephyr_port_ota_update

#ifdef __cplusplus
}
#endif
