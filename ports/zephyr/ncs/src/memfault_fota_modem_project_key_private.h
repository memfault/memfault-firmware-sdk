#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Internal accessor for the modem firmware FOTA project key, shared between the NCS and
//! nRF Cloud override FOTA backends. The public setter lives in memfault/ports/zephyr/fota.h.

#ifdef __cplusplus
extern "C" {
#endif

//! @brief Resolve the Memfault project key used for modem firmware FOTA checks.
//!
//! Returns the runtime override set via memfault_zephyr_fota_modem_project_key_set() when one is
//! configured, otherwise falls back to CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY. Returns NULL when
//! neither is set.
const char *memfault_zephyr_fota_modem_project_key_get(void);

#ifdef __cplusplus
}
#endif
