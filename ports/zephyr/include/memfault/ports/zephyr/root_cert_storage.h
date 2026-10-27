#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
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

  // RSA cert id range: [kMemfaultRootCert_Base + 1, kMemfaultRootCert_RsaMaxIndex). Only
  // installed when RSA endpoints are active.
  kMemfaultRootCert_DigicertRootG2 = kMemfaultRootCert_Base + 1,
  kMemfaultRootCert_AmazonRootCa1,
  kMemfaultRootCert_RsaMaxIndex,

  // ECDSA cert id range: [kMemfaultRootCert_EcdsaBase + 1, kMemfaultRootCert_EcdsaMaxIndex).
  // Offset well clear of the RSA range above (and the reserved deprecated-cert range in
  // deprecated_root_cert.h).
  kMemfaultRootCert_EcdsaBase = kMemfaultRootCert_Base + 10,
  kMemfaultRootCert_DigicertTlsEccP384RootG5 = kMemfaultRootCert_EcdsaBase + 1,
  kMemfaultRootCert_EcdsaMaxIndex,
} eMemfaultRootCert;

#if defined(CONFIG_MEMFAULT_HTTP_USE_ECDSA_ENDPOINTS)
  #define MEMFAULT_ROOT_CERT_ACTIVE_BASE kMemfaultRootCert_EcdsaBase
  #define MEMFAULT_ROOT_CERT_ACTIVE_MAX_INDEX kMemfaultRootCert_EcdsaMaxIndex

  #define MEMFAULT_ROOT_CERTS_ID_LIST kMemfaultRootCert_DigicertTlsEccP384RootG5,
  // DigicertRootG2 and AmazonRootCa1 are only used when RSA endpoints are active. Clean them up
  // if a device previously used RSA endpoints and has since switched to ECDSA endpoints.
  #define MEMFAULT_REMOVE_UNUSED_ROOT_CERTS()                              \
    do {                                                                   \
      memfault_root_cert_storage_remove(kMemfaultRootCert_DigicertRootG2); \
      memfault_root_cert_storage_remove(kMemfaultRootCert_AmazonRootCa1);  \
    } while (0)
#else
  #define MEMFAULT_ROOT_CERT_ACTIVE_BASE kMemfaultRootCert_Base
  #define MEMFAULT_ROOT_CERT_ACTIVE_MAX_INDEX kMemfaultRootCert_RsaMaxIndex

  #define MEMFAULT_ROOT_CERTS_ID_LIST \
    kMemfaultRootCert_DigicertRootG2, kMemfaultRootCert_AmazonRootCa1,
  // DigicertTlsEccP384RootG5 is only used when ECDSA endpoints are active. Clean it up if a
  // device previously used ECDSA endpoints and has since switched to RSA endpoints.
  #define MEMFAULT_REMOVE_UNUSED_ROOT_CERTS() \
    memfault_root_cert_storage_remove(kMemfaultRootCert_DigicertTlsEccP384RootG5)
#endif

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
