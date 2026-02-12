//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/nrfconnect_port/http.h"
#include "memfault/nrfconnect_port/fota.h"
#include "memfault/nrfconnect_port/coap.h"

#include "memfault/components.h"

#include "memfault/ports/zephyr/root_cert_storage.h"

#include "net/downloader.h"
#include "net/fota_download.h"

#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)
// clang-format on

#if !defined(CONFIG_DOWNLOADER)
  #error "CONFIG_DOWNLOADER=y is required to use the Memfault FOTA integration"
#endif

//! Note: A small patch is needed to nrf in order to enable
//! as of the latest SDK release (nRF Connect SDK v1.4.x)
//! See https://mflt.io/nrf-fota for more details.
#if CONFIG_DOWNLOADER_MAX_FILENAME_SIZE < 400
  #warning "CONFIG_DOWNLOADER_MAX_FILENAME_SIZE must be >= 400"

  #if CONFIG_DOWNLOADER_STACK_SIZE < 1600
    #warning "CONFIG_DOWNLOADER_STACK_SIZE must be >= 1600"
  #endif

  #error \
    "DOWNLOADER_MAX_FILENAME_SIZE range may need to be extended in nrf/subsys/net/lib/download_client/Kconfig"
#endif

#if defined(CONFIG_MEMFAULT_SOC_SERIES_NRF91)

  #if CONFIG_MEMFAULT_FOTA_HTTP_FRAG_SIZE > 1024
    #warning "nRF91 modem TLS secure socket buffer limited to 2kB"
    #error "Must use CONFIG_MEMFAULT_FOTA_HTTP_FRAG_SIZE=1024 for FOTA to work reliably"
  #endif

#endif

// The OTA download URL is allocated to this pointer, and must be freed when the
// FOTA download ends.
static char *s_download_url = NULL;

static void prv_fota_url_cleanup(void) {
  MEMFAULT_LOG_DEBUG("Freeing download URL");

#if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  memfault_zephyr_port_coap_release_download_url(&s_download_url);
#else
  memfault_zephyr_port_release_download_url(&s_download_url);
#endif
}

#if !CONFIG_MEMFAULT_FOTA_DOWNLOAD_CALLBACK_CUSTOM
void memfault_fota_download_callback(const struct fota_download_evt *evt) {
  switch (evt->id) {
    case FOTA_DOWNLOAD_EVT_FINISHED:
      MEMFAULT_LOG_INFO("OTA Complete, resetting to install update!");
      // not necessary to call prv_fota_url_cleanup() here since
      // memfault_platform_reboot() will reset the device.
      memfault_platform_reboot();
      break;
    case FOTA_DOWNLOAD_EVT_ERROR:
    case FOTA_DOWNLOAD_EVT_CANCELLED:
      MEMFAULT_LOG_ERROR("FOTA download error: rv=%d", evt->id);
      prv_fota_url_cleanup();
    default:
      break;
  }
}
#endif

// Install this callback wrapper to permit user-supplied callback (when
// CONFIG_MEMFAULT_FOTA_DOWNLOAD_CALLBACK_CUSTOM=y), but to ensure any cleanup
// is done.
static void prv_fota_download_callback_wrapper(const struct fota_download_evt *evt) {
  // May not return if OTA was successful
  memfault_fota_download_callback(evt);

  switch (evt->id) {
    // For any case where the download client has exited, free the OTA URL
    case FOTA_DOWNLOAD_EVT_ERROR:
    case FOTA_DOWNLOAD_EVT_CANCELLED:
    case FOTA_DOWNLOAD_EVT_FINISHED:
      prv_fota_url_cleanup();
      break;
    default:
      break;
  }
}

static const int s_memfault_fota_certs[] = { kMemfaultRootCert_DigicertRootG2,
                                             kMemfaultRootCert_AmazonRootCa1,
                                             kMemfaultRootCert_DigicertRootCa };

int memfault_fota_start(void) {
  // Note: The download URL is allocated on the heap and must be freed when done
#if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  int rv = memfault_zephyr_port_coap_get_download_url(&s_download_url);
#else
  int rv = memfault_zephyr_port_get_download_url(&s_download_url);
#endif
  if (rv <= 0) {
    return rv;
  }
  MEMFAULT_LOG_DEBUG("Allocated new FOTA download URL");

  MEMFAULT_ASSERT(s_download_url != NULL);

  MEMFAULT_LOG_INFO("FOTA Update Available. Starting Download with URL: %s", s_download_url);
  rv = fota_download_init(&prv_fota_download_callback_wrapper);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("FOTA init failed, rv=%d", rv);
    goto cleanup;
  }

  // Find first '/' after the host to split URL into host and filepath components for FOTA API
  char *file = strchr(s_download_url, '/');
  if (!file) {
    MEMFAULT_LOG_ERROR("Invalid URL format");
    rv = -1;
    goto cleanup;
  }

  // Insert a null terminator to split host and file, so we avoid additional memory allocations
  // Note: this will not effect the free()-ing of the original URL string
  *file = '\0';
  file++;  // file now points to the path after the first '/'

  // Assign a new pointer, indicating we have isolated the host
  const char *host = s_download_url;

  MEMFAULT_LOG_DEBUG("Split URL - host: '%s', file: '%s'", host, file);

  // NCS 2.6 introduced a new API to support multiple certificates, so no need to iterate through
  // to find a matching one
  rv =
    fota_download_any(host, file, s_memfault_fota_certs, MEMFAULT_ARRAY_SIZE(s_memfault_fota_certs),
                      0 /* pdn_id */, CONFIG_MEMFAULT_FOTA_HTTP_FRAG_SIZE);

  if (rv != 0) {
    MEMFAULT_LOG_ERROR("FOTA start failed, rv=%d", rv);
    goto cleanup;
  }

  MEMFAULT_LOG_INFO("FOTA In Progress");
  rv = 1;

cleanup:
  // any error code other than 1 or 0 indicates an error
  if ((rv != 1) && (rv != 0)) {
    // free the allocated s_download_url
    prv_fota_url_cleanup();
  }
  return rv;
}
