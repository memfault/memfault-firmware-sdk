//! @file
//!
//! @brief
//! A sanity check to ensure the PEM & DER format of each cert exported match

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <stddef.h>
  #include <stdint.h>
  #include <stdlib.h>
  #include <string.h>

  #include "memfault/http/root_certs.h"
}

TEST_GROUP(MemfaultRootCert){
  void setup() { }
  void teardown() { }
};

static char prv_get_char_from_word(uint32_t word, int offset) {
  const char base64_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const uint8_t base64_mask = 0x3f; // one char per 6 bits
  return base64_table[(word >> (offset * 6)) & base64_mask];
}

static char *prv_der_to_pem(const uint8_t *der, size_t der_len) {
  const char *cert_header = "-----BEGIN CERTIFICATE-----\n";
  const char *cert_footer = "-----END CERTIFICATE-----\n";
  const size_t header_footer_len = strlen(cert_header) + strlen(cert_footer) + 1;

  // each 3 byte binary block is encoded as 4 base64 characters
  const uint32_t pem_len = 4 * ((der_len + 2) / 3) + header_footer_len;
  char *pem = (char *)calloc(1, pem_len);
  CHECK(pem != NULL);

  int pem_idx = sprintf(pem, "%s", cert_header);

  for (size_t der_idx = 0; der_idx < der_len;  der_idx += 3) {
    const uint32_t byte0 = der[der_idx];
    const uint32_t byte1 = ((der_idx + 1) < der_len) ? der[der_idx + 1] : 0;
    const uint32_t byte2 = ((der_idx + 2) < der_len) ? der[der_idx + 2] : 0;
    const uint32_t triple = (byte0 << 16) + (byte1 << 8) + byte2;

    pem[pem_idx++] = prv_get_char_from_word(triple, 3);
    pem[pem_idx++] = prv_get_char_from_word(triple, 2);
    pem[pem_idx++] = ((der_idx + 1) < der_len) ? prv_get_char_from_word(triple, 1) : '=';
    pem[pem_idx++] = ((der_idx + 2) < der_len) ? prv_get_char_from_word(triple, 0) : '=';
  }

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
