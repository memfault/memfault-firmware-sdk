#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief Internal FOTA component registry API, see memfault/nrfconnect_port/fota_component.h

#include "memfault/nrfconnect_port/fota_component.h"

#ifdef __cplusplus
extern "C" {
#endif

//! @return The registered component with @p software_type, or NULL if there is none
const sMemfaultFotaComponent *memfault_fota_component_find(const char *software_type);

//! Fetches (but does not start) the download URL of a pending update for @p component, over nRF
//! Cloud CoAP with CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP=y and HTTPS otherwise. The caller owns *url
//! and releases it with memfault_zephyr_port_coap_release_download_url() or
//! memfault_zephyr_port_release_download_url() respectively.
//!
//! @return 1 if an update is available (*url is set), 0 if not, or a negative errno on failure
//!         (including one from the component's version_get())
int memfault_fota_component_url_get(const sMemfaultFotaComponent *component, char **url);

//! Calls @p check for each registered component, stopping at the first one with an update.
//!
//! kMemfaultFotaComponentInstall_None components are skipped. A failure for one component is
//! logged and the others are still checked, except for a lost connection (-ETIMEDOUT, -ENOTCONN),
//! which ends the scan. -ENOTSUP is skipped without a log.
//!
//! @param check Returns 1 if the component has an update, 0 if not, or a negative errno
//! @return 1 if a component has an update, 0 if none do, or a connection error
int memfault_fota_component_updates_check(int (*check)(const sMemfaultFotaComponent *component,
                                                       void *ctx),
                                          void *ctx);

#ifdef __cplusplus
}
#endif
