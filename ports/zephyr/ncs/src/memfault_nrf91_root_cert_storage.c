//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! When using the nRF9160, certificates cannot be added via Zephyrs
//! tls_credential_add() API. Instead they need to be added using the
//! modem management API which is what this port does.

#include <string.h>

#include "memfault/ports/zephyr/root_cert_storage.h"

//! Between nrf-connect-sdk v1.2.1 and v1.4 the locations of some headers changed.
//! We use __has_include() to support both paths so the port works with either SDK
//! version.
#if __has_include("zephyr/types.h")
  #include "zephyr/types.h"
#endif

#if __has_include("modem/modem_key_mgmt.h")
  #include "modem/modem_key_mgmt.h"
#else
  #include <modem_key_mgmt.h>
#endif

#include "memfault/core/debug_log.h"
#include "memfault/ports/ncs/version.h"

int memfault_root_cert_storage_add(eMemfaultRootCert cert_id, const char *cert,
                                   size_t cert_length) {
  bool exists;

// Note: modem_key_mgmt_exists() signature changed between nRF Connect SDK 1.7 & 1.8
//   https://github.com/nrfconnect/sdk-nrf/pull/5631
#if MEMFAULT_NCS_VERSION_GT(1, 7)
  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
#else
  uint8_t unused;
  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists, &unused);
#endif

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
// Note: modem_key_mgmt_exists() signature changed between nRF Connect SDK 1.7 & 1.8
//   https://github.com/nrfconnect/sdk-nrf/pull/5631
#if MEMFAULT_NCS_VERSION_GT(1, 7)
  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
#else
  uint8_t unused;
  int err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists, &unused);
#endif

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
