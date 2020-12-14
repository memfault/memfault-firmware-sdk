#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Zephyr specific http utility for interfacing with Memfault HTTP utilities

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Installs the root CAs Memfault uses (the certs in "memfault/http/root_certs.h")
//!
//! @note: MUST be called prior to LTE network init (calling NRF's lte_lc_init_and_connect())
//!  for the settings to take effect
//!
//! @return 0 on success, else error code
int memfault_nrfconnect_port_install_root_certs(void);

//! Sends diagnostic data to the Memfault cloud chunks endpoint
//!
//! @note This function is blocking and will return once posting of chunk data has completed.
//!
//! For more info see https://mflt.io/data-to-cloud
int memfault_nrfconnect_port_post_data(void);

#ifdef __cplusplus
}
#endif
