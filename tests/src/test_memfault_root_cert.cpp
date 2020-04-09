//! @file
//!
//! @brief
//! A sanity check to ensure the PEM & DER format of each cert exported match

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"


#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "memfault/http/root_certs.h"
#include "memfault/util/base64.h"

TEST_GROUP(MemfaultRootCert){
  void setup() { }
  void teardown() { }
};

static char *prv_der_to_pem(const uint8_t *der, size_t der_len) {
  const char *cert_header = "-----BEGIN CERTIFICATE-----\n";
  const char *cert_footer = "-----END CERTIFICATE-----\n";
  const size_t header_footer_len = strlen(cert_header) + strlen(cert_footer) + 1;

  // each 3 byte binary block is encoded as 4 base64 characters
  const size_t encode_len = MEMFAULT_BASE64_ENCODE_LEN(der_len);
  const uint32_t pem_len = encode_len + header_footer_len;
  char *pem = (char *)calloc(1, pem_len);
  CHECK(pem != NULL);

  int pem_idx = sprintf(pem, "%s", cert_header);
  memfault_base64_encode(der, der_len, &pem[pem_idx]);
  pem_idx += encode_len;
  sprintf(&pem[pem_idx], "%s", cert_footer);
  return pem;
}

TEST(MemfaultRootCert, Test_cert_dst_ca_x3) {
  char *pem = prv_der_to_pem(g_memfault_cert_dst_ca_x3, g_memfault_cert_dst_ca_x3_len);
  STRCMP_EQUAL(MEMFAULT_ROOT_CERTS_DST_ROOT_CA_X3, pem);
  free(pem);
}

TEST(MemfaultRootCert, Test_cert_digicert_global_root_ca) {
  char *pem = prv_der_to_pem(g_memfault_cert_digicert_global_root_ca,
                             g_memfault_cert_digicert_global_root_ca_len);
  STRCMP_EQUAL(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA, pem);
  free(pem);
}

TEST(MemfaultRootCert, Test_cert_digicert_global_root_g2) {
  char *pem = prv_der_to_pem(g_memfault_cert_digicert_global_root_g2,
                             g_memfault_cert_digicert_global_root_g2_len);
  STRCMP_EQUAL(MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2, pem);
  free(pem);
}

TEST(MemfaultRootCert, Test_cert_baltimore_cybertrust_root) {
  char *pem = prv_der_to_pem(g_memfault_cert_baltimore_cybertrust_root,
                             g_memfault_cert_baltimore_cybertrust_root_len);
  STRCMP_EQUAL(MEMFAULT_ROOT_CERTS_BALTIMORE_CYBERTRUST_ROOT, pem);
  free(pem);
}

TEST(MemfaultRootCert, Test_cert_amazon_root_ca1) {
  char *pem = prv_der_to_pem(g_memfault_cert_amazon_root_ca1,
                             g_memfault_cert_amazon_root_ca1_len);
  STRCMP_EQUAL(MEMFAULT_ROOT_CERTS_AMAZON_ROOT_CA1, pem);
  free(pem);
}
