//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <string.h>
#include <zephyr/ztest.h>

#include "memfault/http/root_certs.h"
#include "memfault/ports/zephyr/deprecated_root_cert.h"
#include "memfault/ports/zephyr/http.h"
#include "memfault/ports/zephyr/root_cert_storage.h"

typedef struct {
  eMemfaultRootCert cert_id;
  const char *cert;
  size_t cert_len;
} sCertAddCall;

static sCertAddCall s_add_calls[4];
static size_t s_add_call_count;

static eMemfaultRootCert s_remove_calls[4];
static size_t s_remove_call_count;

int memfault_root_cert_storage_add(eMemfaultRootCert cert_id, const char *cert,
                                   size_t cert_length) {
  zassert_true(s_add_call_count < ARRAY_SIZE(s_add_calls), "Too many add calls");

  s_add_calls[s_add_call_count].cert_id = cert_id;
  s_add_calls[s_add_call_count].cert = cert;
  s_add_calls[s_add_call_count].cert_len = cert_length;
  s_add_call_count++;
  return 0;
}

int memfault_root_cert_storage_remove(eMemfaultRootCert cert_id) {
  zassert_true(s_remove_call_count < ARRAY_SIZE(s_remove_calls), "Too many remove calls");

  s_remove_calls[s_remove_call_count++] = cert_id;
  return 0;
}

static void *setup_test(void) {
  memset(s_add_calls, 0, sizeof(s_add_calls));
  s_add_call_count = 0;
  memset(s_remove_calls, 0, sizeof(s_remove_calls));
  s_remove_call_count = 0;
  return NULL;
}

ZTEST(memfault_platform_http, test_install_root_certs) {
  const int rv = memfault_zephyr_port_install_root_certs();
  zassert_equal(rv, 0, "Expected root cert installation to succeed");

  const size_t expected_remove_count = (size_t)(kMemfaultDeprecatedRootCert_MaxIndex -
                                                kMemfaultDeprecatedRootCert_DuplicateAmazonRootCa1);
  zassert_equal(s_remove_call_count, expected_remove_count, "Unexpected number of remove calls");

  for (size_t i = 0; i < expected_remove_count; i++) {
    const eMemfaultRootCert expected_id =
      (eMemfaultRootCert)(kMemfaultDeprecatedRootCert_DuplicateAmazonRootCa1 + i);
    zassert_equal(s_remove_calls[i], expected_id, "Unexpected cert id removed at index %d", (int)i);
  }

  static const eMemfaultRootCert expected_add_ids[] = {
    kMemfaultRootCert_DigicertRootG2,
    kMemfaultRootCert_AmazonRootCa1,
    kMemfaultRootCert_DigicertRootCa,
  };

#if defined(CONFIG_MEMFAULT_TLS_CERTS_USE_DER)
  static const uint8_t *const expected_add_certs[] = {
    g_memfault_cert_digicert_global_root_g2,
    g_memfault_cert_amazon_root_ca1,
    g_memfault_cert_digicert_global_root_ca,
  };
  size_t expected_add_cert_lens[3];
  expected_add_cert_lens[0] = g_memfault_cert_digicert_global_root_g2_len;
  expected_add_cert_lens[1] = g_memfault_cert_amazon_root_ca1_len;
  expected_add_cert_lens[2] = g_memfault_cert_digicert_global_root_ca_len;
#elif defined(CONFIG_MEMFAULT_TLS_CERTS_USE_PEM)
  static const uint8_t *const expected_add_certs[] = {
    (const uint8_t *)MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2,
    (const uint8_t *)MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1,
    (const uint8_t *)MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA,
  };
  static const size_t expected_add_cert_lens[] = {
    sizeof(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2),
    sizeof(MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1),
    sizeof(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA),
  };
#else
  #error "Unsupported cert format"
#endif

  zassert_equal(s_add_call_count, ARRAY_SIZE(expected_add_ids), "Unexpected number of add calls");

  for (size_t i = 0; i < ARRAY_SIZE(expected_add_ids); i++) {
    zassert_equal(s_add_calls[i].cert_id, expected_add_ids[i],
                  "Unexpected cert id added at index %d", (int)i);
    zassert_equal(s_add_calls[i].cert_len, expected_add_cert_lens[i],
                  "Unexpected cert length at index %d", (int)i);
    zassert_mem_equal(s_add_calls[i].cert, expected_add_certs[i], expected_add_cert_lens[i],
                      "Unexpected cert contents at index %d", (int)i);
  }
}

ZTEST_SUITE(memfault_platform_http, NULL, setup_test, NULL, NULL, NULL);
