//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Example code that uses libcurl to fetch the latest release for an example
//! device.
//!
//! Compiling: In Memfault UI, navigate to Settings->General Settings and
//!  Copy/Paste "Project API Key" into PROJECT_KEY below:
//!
//!  $ gcc get_latest.c -lcurl -lssl -lcrypto -Wall -DMEMFAULT_PROJECT_KEY=\"PROJECT_KEY\"
//!
//! NOTE: Requires libcurl and libopenssl. On Debian, 'sudo apt install -y libcurl4-openssl-dev'
//!
//!  Usage:
//!  $ ./a.out <device_serial> <hardware_version> <software_type> <current_version>
//!
//! NOTE: The device_serial, hardware_version, software_type, and
//! current_version are mandatory, and must be valid for the test device. When
//! testing cohort release activation, make sure the device_serial is present in
//! the test cohort.

#define _GNU_SOURCE  // for asprintf
#include <assert.h>
#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../../components/include/memfault/http/root_certs.h"

#ifndef MEMFAULT_PROJECT_KEY
  #error "MEMFAULT_PROJECT_KEY definition required"
#endif

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

static int prv_get_latest_release(const char *device_serial, const char *hardware_version,
                                  const char *software_type, const char *current_version,
                                  bool verbose) {
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;

  slist1 = NULL;
  slist1 = curl_slist_append(slist1, "Memfault-Project-Key:" MEMFAULT_PROJECT_KEY);

  hnd = curl_easy_init();

  char *device_serial_ = curl_easy_escape(hnd, device_serial, 0);
  char *hardware_version_ = curl_easy_escape(hnd, hardware_version, 0);
  char *software_type_ = curl_easy_escape(hnd, software_type, 0);
  char *current_version_ = curl_easy_escape(hnd, current_version, 0);

  char *url;
  int rv = asprintf(&url,
                    "https://device.memfault.com/api/v0/releases/latest/url"
                    "?device_serial=%s&hardware_version=%s&software_type=%s&current_version=%s",
                    device_serial_, hardware_version_, software_type_, current_version_);
  curl_free(device_serial_);
  curl_free(hardware_version_);
  curl_free(software_type_);
  curl_free(current_version_);
  assert(rv >= 0);
  curl_easy_setopt(hnd, CURLOPT_URL, url);
  free(url);

  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/8.5.0");
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
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
    if ((http_code != 200) && (http_code != 204)) {
      ret = http_code;
    }
  }

  curl_easy_cleanup(hnd);
  hnd = NULL;
  curl_slist_free_all(slist1);
  slist1 = NULL;

  return (int)ret;
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("Usage: %s <device_serial> <hardware_version> <software_type> <current_version>\n",
           argv[0]);
    return 1;
  }

  const char *device_serial = argv[1];
  const char *hardware_version = argv[2];
  const char *software_type = argv[3];
  const char *current_version = argv[4];

  int rv = prv_get_latest_release(device_serial, hardware_version, software_type, current_version,
                                  true /* verbose */);
  if (rv != 0) {
    printf("\n\nERROR: Get latest release failed, rv=%d\n", rv);
  } else {
    printf("\n\nSuccess!\n\n");
  }

  return rv;
}
