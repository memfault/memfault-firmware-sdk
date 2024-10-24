//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! When using the nRF9160, certificates cannot be added via Zephyrs
//! tls_credential_add() API. Instead they need to be added using the
//! modem management API which is what this port does.

#include <string.h>

#include "memfault/core/debug_log.h"
#include "memfault/ports/zephyr/root_cert_storage.h"
#include "modem/modem_key_mgmt.h"
#include "zephyr/types.h"

int memfault_root_cert_storage_add(eMemfaultRootCert cert_id, const char *cert,
                                   size_t cert_length) {
  bool exists;

  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);

  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to check if cert %d exists in storage, rv=%d", cert_id, err);
    return err;
  }

  if (exists) {
    err = modem_key_mgmt_cmp(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, cert_length - 1);
    if (err == 0) {
      MEMFAULT_LOG_DEBUG("Cert %d in storage is up-to-date", cert_id);
      return 0;
    }
    // if key is mismatched, continue with writing over the entry with the correct key
    MEMFAULT_LOG_INFO("Cert %d in storage is not up-to-date", cert_id);
  }

  MEMFAULT_LOG_INFO("Installing Root CA %d", cert_id);
  err = modem_key_mgmt_write(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, strlen(cert));
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to provision certificate, err %d", err);
  }

  return err;
}

int memfault_root_cert_storage_remove(eMemfaultRootCert cert_id) {
  bool exists;
  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);

  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to check if cert %d exists in storage, rv=%d", cert_id, err);
    return err;
  }

  if (!exists) {
    MEMFAULT_LOG_DEBUG("Cert %d not found in storage, skipping removal", cert_id);
    return 0;
  }

  MEMFAULT_LOG_INFO("Removing Root CA %d", cert_id);
  err = modem_key_mgmt_delete(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to delete cert, err %d", err);
  }
  return err;
}
