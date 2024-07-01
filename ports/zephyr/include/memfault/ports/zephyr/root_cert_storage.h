#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A wrapper around root certificate storage with Zephyr since different modules may use different
//! implementations. For example, the nRF9160 has its own offloaded storage on the modem whereas a
//! external LTE modem may use local Mbed TLS storage on the device.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// These cert ids are used to identify certs when using them for TLS handshakes.
// They persist in modem flash storage with the stored cert. If a required cert is already
// stored, it should be checked to make sure it is up-to-date, and any deprecated certs should
// be removed. Deprecated cert ids are kept in deprecated_root_cert.h
typedef enum {
  // arbitrarily high base so as not to conflict with id used for other certs in use by the system
  kMemfaultRootCert_Base = 1000,
  // Keep this index first, or update the code that references it
  kMemfaultRootCert_DigicertRootG2,
  kMemfaultRootCert_AmazonRootCa1,
  kMemfaultRootCert_DigicertRootCa,
  // Must be last, used to track number of root certs in use
  kMemfaultRootCert_MaxIndex,
} eMemfaultRootCert;

//! Adds specified certificate to backing store
//!
//! @param cert_id Identifier to be used to reference the certificate
//! @param cert root certificate to add to storage (
//! @param cert_length Length of PEM certificate
//!
//! @return 0 on success or if the cert was already loaded, else error code
int memfault_root_cert_storage_add(eMemfaultRootCert cert_id, const char *cert, size_t cert_length);

//! Remove specified certificate from backing store
//!
//! @param cert_id Identifier to be used to reference the certificate
//!
//! @return 0 on success or if the cert was not already loaded, else error code
int memfault_root_cert_storage_remove(eMemfaultRootCert cert_id);

#ifdef __cplusplus
}
#endif
