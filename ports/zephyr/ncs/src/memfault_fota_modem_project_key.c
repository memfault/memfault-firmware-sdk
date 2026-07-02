//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Shared storage and accessors for the modem firmware FOTA project key.

#include "memfault/components.h"
#include "memfault/ports/zephyr/fota.h"
#include "memfault_fota_modem_project_key_private.h"

static const char *s_memfault_fota_modem_project_key_override = NULL;

void memfault_zephyr_fota_modem_project_key_set(const char *key) {
  s_memfault_fota_modem_project_key_override = key;
}

const char *memfault_zephyr_fota_modem_project_key_get(void) {
  if (s_memfault_fota_modem_project_key_override) {
    return s_memfault_fota_modem_project_key_override;
  }
  // sizeof > 1 means the string is non-empty (accounts for the null terminator)
  return (sizeof(CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY) > 1) ?
           CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY :
           NULL;
}
