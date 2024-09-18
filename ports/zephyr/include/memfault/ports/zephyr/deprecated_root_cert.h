#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include "memfault/core/compiler.h"
#include "memfault/ports/zephyr/root_cert_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  // The Baltimore Cyber Trust Root was the actual deprecated root
  // but its previous cert id, 1003, is now the cert id for the
  // kMemfaultRootCert_AmazonRootCa1. Therefore, modems will have a
  // duplicate Amazon cert that should be removed
  kMemfaultDeprecatedRootCert_DuplicateAmazonRootCa1 = 1004,
  kMemfaultDeprecatedRootCert_MaxIndex,
} eMemfaultDeprecatedRootCert;

MEMFAULT_STATIC_ASSERT((kMemfaultRootCert_MaxIndex - 1) <
                         kMemfaultDeprecatedRootCert_DuplicateAmazonRootCa1,
                       "Overlap detected between in-use root certs and deprecated root certs.");

#ifdef __cplusplus
}
#endif
