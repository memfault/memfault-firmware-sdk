//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Example code that uses libcurl to post memfault the memfault test data from:
//!   https://mflt.io/chunks-api-integration
//!
//! Compiling:
//!  In Memfault UI, navigate to Settings->General Settings and Copy/Paste "Project API Key"
//!  into PROJECT_KEY below:
//!
//!  $ gcc libcurl_example.c -lcurl -lssl -lcrypto -Wall -DMEMFAULT_PROJECT_KEY=\"PROJECT_KEY\"
//!
//! NOTE: Requires libcurl and libopenssl. On Debian, 'sudo apt install -y libcurl4-openssl-dev'
//!
//!  Usage:
//!  $ ./a.out
//!  $ Chunk successfully sent!

#include <curl/curl.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../components/include/memfault/http/root_certs.h"

#ifndef MEMFAULT_PROJECT_KEY
  #error "MEMFAULT_PROJECT_KEY definition required"
#endif

#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>

//! Provide the CA cert from openssl directly as an in-memory cert, instead of
//! retrieving from the default system certificate store.
//!
//! This strategy is adapted from the example here:
//! https://github.com/curl/curl/blob/master/docs/examples/cacertinmem.c
static CURLcode prv_install_root_certs(CURL *curl, void *sslctx, void *param) {
  CURLcode rv = CURLE_ABORTED_BY_CALLBACK;

  static const char mypem[] = MEMFAULT_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2;

  BIO *cbio = BIO_new_mem_buf(mypem, sizeof(mypem));
  X509_STORE *cts = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
  int i;
  STACK_OF(X509_INFO) * inf;
  (void)curl;
  (void)param;

  if (!cts || !cbio) {
    return rv;
  }

  inf = PEM_X509_INFO_read_bio(cbio, NULL, NULL, NULL);

  if (!inf) {
    BIO_free(cbio);
    return rv;
  }

  for (i = 0; i < sk_X509_INFO_num(inf); i++) {
    X509_INFO *itmp = sk_X509_INFO_value(inf, i);
    if (itmp->x509) {
      X509_STORE_add_cert(cts, itmp->x509);
    }
    if (itmp->crl) {
      X509_STORE_add_crl(cts, itmp->crl);
    }
  }

  sk_X509_INFO_pop_free(inf, X509_INFO_free);
  BIO_free(cbio);

  rv = CURLE_OK;
  return rv;
}

static int prv_post_chunk(const char *device_serial, const void *chunk, size_t chunk_len,
                          bool verbose) {
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;

  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "Memfault-Project-Key:" MEMFAULT_PROJECT_KEY);
  slist1 = curl_slist_append(slist1, "Content-Type: application/octet-stream");

  hnd = curl_easy_init();
  char dest_url[512];
  snprintf(dest_url, sizeof(dest_url), "https://chunks.memfault.com/api/v0/chunks/%s",
           device_serial);

  curl_easy_setopt(hnd, CURLOPT_URL, dest_url);
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, chunk);
  curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)chunk_len);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/8.5.0");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  /* Turn off the default CA locations, otherwise libcurl loads CA
   * certificates from the locations that were detected/specified at
   * build-time
   */
  curl_easy_setopt(hnd, CURLOPT_CAINFO, NULL);
  curl_easy_setopt(hnd, CURLOPT_CAPATH, NULL);

  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 1L);
  curl_easy_setopt(hnd, CURLOPT_SSLCERTTYPE, "PEM");
  curl_easy_setopt(hnd, CURLOPT_SSL_CTX_FUNCTION, prv_install_root_certs);

  if (verbose) {
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
  }

  ret = curl_easy_perform(hnd);

  if (ret == 0) {
    long http_code = 0;
    curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &http_code);
    ret = !(http_code == 202);
  }

  curl_easy_cleanup(hnd);
  hnd = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;

  return (int)ret;
}

int main(int argc, char *argv[]) {
  // This is the example chunk from:
  //   https://docs.memfault.com/docs/embedded/test-patterns-for-chunks-endpoint#event-message-encoded-in-a-single-chunk
  const uint8_t chunk[] = { 0x08, 0x02, 0xa7, 0x02, 0x01, 0x03, 0x01, 0x07, 0x6a, 0x54, 0x45,
                            0x53, 0x54, 0x53, 0x45, 0x52, 0x49, 0x41, 0x4c, 0x0a, 0x6d, 0x74,
                            0x65, 0x73, 0x74, 0x2d, 0x73, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72,
                            0x65, 0x09, 0x6a, 0x31, 0x2e, 0x30, 0x2e, 0x30, 0x2d, 0x74, 0x65,
                            0x73, 0x74, 0x06, 0x6d, 0x74, 0x65, 0x73, 0x74, 0x2d, 0x68, 0x61,
                            0x72, 0x64, 0x77, 0x61, 0x72, 0x65, 0x04, 0xa1, 0x01, 0xa1, 0x72,
                            0x63, 0x68, 0x75, 0x6e, 0x6b, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x5f,
                            0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x01, 0x31, 0xe4 };

  int rv = prv_post_chunk("TESTSERIAL", chunk, sizeof(chunk), true /* verbose */);
  if (rv != 0) {
    printf("\n\nERROR: Chunk post failed, rv=%d\n", rv);
  } else {
    printf("\n\nChunk successfully sent!\n\n");
  }

  return rv;
}
