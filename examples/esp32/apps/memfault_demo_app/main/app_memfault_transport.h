//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//! Initializes any components needed for configured transport
//!
//! Transport is selected via CONFIG_APP_MEMFAULT_TRANSPORT choices
void app_memfault_transport_init(void);

//! Sends all available Memfault chunks over configured transport
//!
//! Transport is selected via CONFIG_APP_MEMFAULT_TRANSPORT choices
//!
//! @return 0 on success or non-zero on error
int app_memfault_transport_send_chunks(void);

#ifdef __cplusplus
}
#endif
