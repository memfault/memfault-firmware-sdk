//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

// clang-format off
#include "memfault/nrfconnect_port/http.h"
#include "memfault/nrfconnect_port/fota.h"

#include "memfault/components.h"

#include "memfault/ports/ncs/version.h"
#include "memfault/ports/zephyr/root_cert_storage.h"

#include "net/download_client.h"
#include "net/fota_download.h"

#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)
// clang-format on

//! Note: A small patch is needed to nrf in order to enable
//! as of the latest SDK release (nRF Connect SDK v1.4.x)
//! See https://mflt.io/nrf-fota for more details.
#if CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE < 400
#warning "CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE must be >= 400"

#if CONFIG_DOWNLOAD_CLIENT_STACK_SIZE < 1600
#warning "CONFIG_DOWNLOAD_CLIENT_STACK_SIZE must be >= 1600"
#endif

#error "DOWNLOAD_CLIENT_MAX_FILENAME_SIZE range may need to be extended in nrf/subsys/net/lib/download_client/Kconfig"
#endif

#if CONFIG_SOC_SERIES_NRF91X

#if CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE > 1024
#warning "nRF91 modem TLS secure socket buffer limited to 2kB"
#error "Must use CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE_1024=y for FOTA to work reliably"
#endif

#endif

// The OTA download URL is allocated to this pointer, and must be freed when the
// FOTA download ends.
static char *s_download_url = NULL;

static void prv_fota_url_cleanup(void) {
  MEMFAULT_LOG_DEBUG("Freeing download URL");
  memfault_zephyr_port_release_download_url(&s_download_url);
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

static const int s_memfault_fota_certs[] = {
  kMemfaultRootCert_DigicertRootG2,
  kMemfaultRootCert_AmazonRootCa1,
  kMemfaultRootCert_DigicertRootCa
};

#if MEMFAULT_NCS_VERSION_GT(2, 3)

int __real_download_client_get(struct download_client *client, const char *host,
                               const struct download_client_cfg *config, const char *file, size_t from);
int __wrap_download_client_get(struct download_client *client, const char *host,
                                const struct download_client_cfg *config, const char *file, size_t from) {
  // In NCS 2.4, there were two significant changes to the download client
  //
  //  1. Connecting to the OTA (download_client_connect) server was made asynchronous and no error
  //     information is exposed to the FOTA client. This makes it impossible to rotate through certificates
  //     like we did for previous releases.
  //        https://github.com/nrfconnect/sdk-nrf/commit/38978c8092#diff-6a4addb1807793400159a4f7592864c3edab770e4919c5708b583115fab54e9aL430
  //
  //  2. Support was added to the download_client (but not the fota_download handler) to support multiple certificates!
  //        https://github.com/nrfconnect/sdk-nrf/commit/0d177a1282a5d9b12c9d5f7d00837d0771f5c2ee
  //
  // In this routine, we intercept the download_client_get() call so we can patch in all the
  // Memfault certificates before the FOTA begins.

  // Copy the existing config
  struct download_client_cfg modified_config = *config;

  // Modify sec_tag_list so it has all clients
  modified_config.sec_tag_list = &s_memfault_fota_certs[0];
  modified_config.sec_tag_count = MEMFAULT_ARRAY_SIZE(s_memfault_fota_certs);

  return __real_download_client_get(client, host, &modified_config, file, from);
}

// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&download_client_get),
                                            __typeof__(&__wrap_download_client_get)) &&
                 __builtin_types_compatible_p(__typeof__(&download_client_get),
                                              __typeof__(&__real_download_client_get)),
               "Error: Wrapped functions does not match original download_client_get function signature");
#endif

int memfault_fota_start(void) {
  // Note: The download URL is allocated on the heap and must be freed when done
  int rv = memfault_zephyr_port_get_download_url(&s_download_url);
  if (rv <= 0) {
    return rv;
  }
  MEMFAULT_LOG_DEBUG("Allocated new FOTA download URL");

  MEMFAULT_ASSERT(s_download_url != NULL);

  MEMFAULT_LOG_INFO("FOTA Update Available. Starting Download!");
  rv = fota_download_init(&prv_fota_download_callback_wrapper);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("FOTA init failed, rv=%d", rv);
    goto cleanup;
  }

  // Note: The nordic FOTA API only supports passing one root CA today. So we cycle through the
  // list of required Root CAs in use by Memfault to find the appropriate one
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_memfault_fota_certs); i++) {

#if MEMFAULT_NCS_VERSION_GT(1, 7)
    // In NCS 1.8 signature was changed "to accept an integer parameter specifying the PDN ID,
    // which replaces the parameter used to specify the APN"
    //
    // https://github.com/nrfconnect/sdk-nrf/blob/v1.8.0/include/net/fota_download.h#L88-L106
    rv = fota_download_start(s_download_url, s_download_url, s_memfault_fota_certs[i],
                             0 /* pdn_id */, 0);
#else // NCS <= 1.7
    // https://github.com/nrfconnect/sdk-nrf/blob/v1.4.1/include/net/fota_download.h#L88-L106
    rv = fota_download_start(s_download_url, s_download_url, s_memfault_fota_certs[i],
                             NULL /* apn */, 0);
#endif
    if (rv == 0) {
      // success -- we are ready to start the FOTA download!
      break;
    }

    if (rv == -EALREADY) {
      MEMFAULT_LOG_INFO("fota_download_start already in progress");
      break;
    }

    // Note: Between releases of Zephyr & NCS the error code returned for TLS handshake failures
    // has changed.
    //
    // NCS < 1.6 returned EOPNOTSUPP
    // NCS >= 1.6 returns ECONNREFUSED.
    //
    // The mapping of errno values has also changed across releases due to:
    // https://github.com/zephyrproject-rtos/zephyr/commit/165def7ea6709f7f0617591f464fb95f711d2ac0
    //
    // Therefore we print a friendly message when an _expected_ failure takes place
    // but retry regardless of error condition in case value returned changes again in the future.
    //
    // Note: For NCS >= 2.4, server connect() has been moved to the download_client thread and TLS
    // error codes are no longer bubbled up here so we do not expect to fall into this logic
    // anymore.  See workaround above in __wrap_download_client_get() for more details.
    if (((errno == EOPNOTSUPP) || (errno == ECONNREFUSED)) &&
        (i != (MEMFAULT_ARRAY_SIZE(s_memfault_fota_certs) - 1))) {
      MEMFAULT_LOG_INFO("fota_download_start likely unsuccessful due to root cert mismatch. "
                        "Trying next certificate.");
    } else {
      MEMFAULT_LOG_ERROR("fota_download_start unexpected failure, errno=%d", (int)errno);
    }
  }
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
