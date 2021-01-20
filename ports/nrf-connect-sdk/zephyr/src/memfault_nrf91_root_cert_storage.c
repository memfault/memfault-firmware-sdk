//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! When using the nRF9160, certificates cannot be added via Zephyrs
//! tls_credential_add() API. Instead they need to be added using the
//! modem management API which is what this port does.

#include "memfault/ports/zephyr/root_cert_storage.h"

#include <string.h>

#include <modem/modem_key_mgmt.h>
#include "memfault/core/debug_log.h"

int memfault_root_cert_storage_add(
    eMemfaultRootCert cert_id, const char *cert, size_t cert_length) {
  bool exists;
  uint8_t unused;

  int err = modem_key_mgmt_exists(cert_id,
                                  MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                                  &exists, &unused);
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to install cert %d, rv=%d\n", cert_id, err);
    return err;
  }

  if (exists) {
    return 0;
  }

  MEMFAULT_LOG_INFO("Installing Root CA %d", cert_id);
  err = modem_key_mgmt_write(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
                             cert, strlen(cert));
  if (err != 0) {
    MEMFAULT_LOG_ERROR("Failed to provision certificate, err %d\n", err);
  }

  return err;
}
