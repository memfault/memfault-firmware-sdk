//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

#include "memfault/nrfconnect_port/fota_component.h"
#include "memfault/ports/zephyr/fota.h"
#include "memfault/ports/zephyr/http.h"

#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  #include "memfault/nrfconnect_port/coap.h"

  // Provides nrf_cloud_download_start(), which routes the FOTA download through the nRF Cloud
  // CoAP proxy resource. Header lives in the nrf_cloud library's common/include dir, which is on
  // the zephyr include path (see nrf_cloud CMakeLists.txt).
  #include <nrf_cloud_download.h>

  // Provides nrf_cloud_sec_tag_get(), the DTLS security tag used for the nRF Cloud CoAP connection.
  #include <net/nrf_cloud.h>
  #include <net/nrf_cloud_coap.h>
#endif

#if defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE) && !defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  #include <net/nrf_cloud.h>
#endif

#include "memfault/components.h"

#include "memfault/ports/zephyr/root_cert_storage.h"

#include "net/downloader.h"
#include "net/fota_download.h"
// clang-format on

#include "memfault_fota_component_private.h"

#if defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE)
  #include <modem/lte_lc.h>
#endif

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
static bool s_modem_download_active;

// Used to prevent concurrent access on s_download_url and FOTA operations. We
// only permit one FOTA operation at a time.
static atomic_t s_fota_busy;

static int prv_fota_lock(void) {
  if (!atomic_cas(&s_fota_busy, 0, 1)) {
    MEMFAULT_LOG_WARN("FOTA check or download already in progress");
    return -EBUSY;
  }
  return 0;
}

//! Releases the lock taken by prv_fota_lock(), unless @p rv == 1: a download started and now owns
//! s_fota_busy (released in the download callback).
static int prv_fota_unlock_unless_started(int rv) {
  if (rv != 1) {
    atomic_clear(&s_fota_busy);
  }
  return rv;
}

static int prv_fota_run_locked(int (*fn)(void)) {
  int rv = prv_fota_lock();
  if (rv != 0) {
    return rv;
  }
  return prv_fota_unlock_unless_started(fn());
}

// Forward declaration: default implementation is below; custom provided by user when
// CONFIG_MEMFAULT_FOTA_DOWNLOAD_CALLBACK_CUSTOM=y.
void memfault_fota_download_callback(const struct fota_download_evt *evt);

static void prv_fota_url_cleanup(void) {
  MEMFAULT_LOG_DEBUG("Freeing download URL");

#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  memfault_zephyr_port_coap_release_download_url(&s_download_url);
#else
  memfault_zephyr_port_release_download_url(&s_download_url);
#endif
}

#if defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE)
static int prv_apply_full_modem_update(void) {
  if (!s_modem_download_active) {
    return 0;
  }

  #if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  (void)nrf_cloud_coap_disconnect();
  #endif
  (void)lte_lc_power_off();

  int rv = nrf_cloud_fota_fmfu_apply();
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("Full modem update apply failed, rv=%d", rv);
  }
  return rv;
}
#endif

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
      // URL is freed by prv_fota_download_callback_wrapper()
      MEMFAULT_LOG_ERROR("FOTA download error: rv=%d", evt->id);
      break;
    default:
      break;
  }
}
#endif

// Install this callback wrapper to permit user-supplied callback (when
// CONFIG_MEMFAULT_FOTA_DOWNLOAD_CALLBACK_CUSTOM=y), but to ensure any cleanup
// is done.
static void prv_fota_download_callback_wrapper(const struct fota_download_evt *evt) {
#if defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE)
  if (evt->id == FOTA_DOWNLOAD_EVT_FINISHED && prv_apply_full_modem_update() != 0) {
    MEMFAULT_LOG_ERROR("Full modem OTA failed; rebooting");
    memfault_platform_reboot();
  } else
#endif
  {
    // May not return if OTA was successful
    memfault_fota_download_callback(evt);
  }

  switch (evt->id) {
    // For any case where the download client has exited, free the OTA URL
    case FOTA_DOWNLOAD_EVT_ERROR:
    case FOTA_DOWNLOAD_EVT_CANCELLED:
    case FOTA_DOWNLOAD_EVT_FINISHED:
#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
      // Mark download as inactive (counterpart to nrf_cloud_download_start()).
      // Needed so future attempts do not permanently fail with -EBUSY.
      nrf_cloud_download_end();
#endif
      prv_fota_url_cleanup();
      s_modem_download_active = false;
      // Must be last: permits a new FOTA check to allocate s_download_url
      atomic_clear(&s_fota_busy);
      break;
    default:
      break;
  }
}

#if !CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
static const int s_memfault_fota_certs[] = { MEMFAULT_ROOT_CERTS_ID_LIST };
#endif

//! Start a FOTA download from a pre-fetched URL. s_download_url must be the allocated URL;
//! on success (rv==1) ownership is retained and freed in the download callback. On error it
//! is freed here.
static int prv_fota_download_start(void) {
  MEMFAULT_ASSERT(s_download_url != NULL);

  MEMFAULT_LOG_INFO("FOTA Update Available. Starting Download with URL: %s", s_download_url);

  // Find first '/' after the host to split URL into host and filepath components for FOTA API
  // Skip past the scheme (e.g. "https://") before searching for the path separator
  char *host_start = strstr(s_download_url, "://");
  host_start = host_start ? host_start + 3 : s_download_url;
  char *file = strchr(host_start, '/');
  if (!file) {
    MEMFAULT_LOG_ERROR("Invalid URL format");
    prv_fota_url_cleanup();
    return -1;
  }

  // Insert a null terminator to split host and file, so we avoid additional memory allocations
  // Note: this will not effect the free()-ing of the original URL string
  *file = '\0';
  file++;  // file now points to the path after the first '/'

  int rv;

#if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  // Route the download through the nRF Cloud CoAP proxy resource. fota_download_any() does a
  // direct HTTPS download and would bypass the CoAP connection entirely; nrf_cloud_download_start()
  // instead sets up a proxy download (DTLS connection ID, JWT auth callback, and a PROXY_URI option
  // pointing at the real firmware URL) and calls fota_download_init() internally.
  //
  // nrf_cloud_coap_transport_proxy_dl_uri_get() prepends "https://" to the host, so pass the bare
  // hostname (host_start) here, not s_download_url which still carries the scheme.
  MEMFAULT_LOG_DEBUG("Split URL - host: '%s', file: '%s'", host_start, file);

  // The proxy download opens its own DTLS connection to the nRF Cloud CoAP server, so it needs the
  // same security tag the CoAP transport uses. Without it the downloader fails to init the
  // transport ("No security tag provided for TLS/DTLS"). Keep sec_tag in scope until the call
  // returns (nrf_cloud_download_start_internal() reads it synchronously).
  const sec_tag_t sec_tag = nrf_cloud_sec_tag_get();
  struct nrf_cloud_download_data dl_data = {
    .type = NRF_CLOUD_DL_TYPE_FOTA,
    .host = host_start,
    .path = file,
    .dl_host_conf = {
      .sec_tag_list = &sec_tag,
      .sec_tag_count = 1,
    },
    .fota = {
      .expected_type = DFU_TARGET_IMAGE_TYPE_ANY,
      .cb = &prv_fota_download_callback_wrapper,
    },
  };
  rv = nrf_cloud_download_start(&dl_data);
#else
  rv = fota_download_init(&prv_fota_download_callback_wrapper);
  if (rv != 0) {
    MEMFAULT_LOG_ERROR("FOTA init failed, rv=%d", rv);
    goto cleanup;
  }

  // host still carries the scheme (e.g. "https://hostname"), which fota_download_any expects
  const char *host = s_download_url;

  MEMFAULT_LOG_DEBUG("Split URL - host: '%s', file: '%s'", host, file);

  // NCS 2.6 introduced a new API to support multiple certificates, so no need to iterate through
  // to find a matching one
  rv =
    fota_download_any(host, file, s_memfault_fota_certs, MEMFAULT_ARRAY_SIZE(s_memfault_fota_certs),
                      0 /* pdn_id */, CONFIG_MEMFAULT_FOTA_HTTP_FRAG_SIZE);
#endif

  if (rv != 0) {
    MEMFAULT_LOG_ERROR("FOTA start failed, rv=%d", rv);
    goto cleanup;
  }

  MEMFAULT_LOG_INFO("FOTA In Progress");
  rv = 1;

cleanup:
  if ((rv != 1) && (rv != 0)) {
    prv_fota_url_cleanup();
    s_modem_download_active = false;
  }
  return rv;
}

//! Checks @p component for an update and, if one is available, starts its download.
//! @return 1 if a download started, 0 if no update is available, <0 on error
static int prv_fota_component_start(const sMemfaultFotaComponent *component) {
  switch (component->install) {
    case kMemfaultFotaComponentInstall_None:
      // Installed by the application, see
      // memfault_zephyr_fota_component_get_download_url()
      return -ENOTSUP;
    case kMemfaultFotaComponentInstall_ModemFull:
#if !defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE)
      MEMFAULT_LOG_ERROR("'%s' requires CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE=y",
                         component->software_type);
      return -ENOTSUP;
#endif
    case kMemfaultFotaComponentInstall_ModemDelta:
    case kMemfaultFotaComponentInstall_Bootloader:
      break;
    default:
      MEMFAULT_LOG_ERROR("'%s' has an invalid install value: %d", component->software_type,
                         (int)component->install);
      return -EINVAL;
  }

  // On success (rv == 1), this module owns s_download_url, as prv_fota_download_start() expects
  int rv = memfault_fota_component_url_get(component, &s_download_url);
  if (rv <= 0) {
    if (rv == 0) {
      MEMFAULT_LOG_INFO("'%s' is up to date", component->software_type);
    }
    return rv;
  }

#if defined(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE)
  if (component->install == kMemfaultFotaComponentInstall_ModemFull) {
    rv = nrf_cloud_fota_fmfu_dev_set(NULL);
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Full modem FOTA init failed, rv=%d", rv);
      prv_fota_url_cleanup();
      return rv;
    }
    s_modem_download_active = true;
  }
#endif

  // Other install types need no setup: the DFU target is detected from the image header
  return prv_fota_download_start();
}

static int prv_fota_component_start_cb(const sMemfaultFotaComponent *component,
                                       MEMFAULT_UNUSED void *ctx) {
  return prv_fota_component_start(component);
}

//! Starts the download of the first registered component with an update.
static int prv_fota_components_start(void) {
  return memfault_fota_component_updates_check(prv_fota_component_start_cb, NULL);
}

int memfault_zephyr_fota_component_start(const char *software_type) {
  const sMemfaultFotaComponent *component = memfault_fota_component_find(software_type);
  if (!component) {
    return -ENOENT;
  }
  int rv = prv_fota_lock();
  if (rv != 0) {
    return rv;
  }
  return prv_fota_unlock_unless_started(prv_fota_component_start(component));
}

int memfault_zephyr_fota_component_get_download_url(const char *software_type, char **url) {
  const sMemfaultFotaComponent *component = memfault_fota_component_find(software_type);
  if (!component) {
    return -ENOENT;
  }
  int rv = prv_fota_lock();
  if (rv != 0) {
    return rv;
  }
  rv = memfault_fota_component_url_get(component, url);
  atomic_clear(&s_fota_busy);
  return rv;
}

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
int memfault_zephyr_fota_modem_start(void) {
  return memfault_zephyr_fota_component_start(CONFIG_MEMFAULT_FOTA_MODEM_SOFTWARE_TYPE);
}

int memfault_zephyr_fota_modem_get_download_url(char **url) {
  return memfault_zephyr_fota_component_get_download_url(CONFIG_MEMFAULT_FOTA_MODEM_SOFTWARE_TYPE,
                                                         url);
}
#endif  // CONFIG_MEMFAULT_FOTA_MODEM_UPDATE

static int prv_fota_app_start(void) {
  // Note: The download URL is allocated on the heap and must be freed when done
#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  int rv = memfault_zephyr_port_coap_get_download_url(&s_download_url);
#else
  int rv = memfault_zephyr_port_get_download_url(&s_download_url);
#endif
  if (rv < 0) {
    return rv;
  }

  if (rv > 0) {
    // Application firmware update available - start download
    return prv_fota_download_start();
  }

  return 0;
}

int memfault_zephyr_fota_app_start(void) {
  return prv_fota_run_locked(prv_fota_app_start);
}

static int prv_fota_start(void) {
  int rv = prv_fota_app_start();
  if (rv != 0) {
    return rv;
  }

  // No app update available: check the components
  return prv_fota_components_start();
}

int memfault_zephyr_fota_start(void) {
  return prv_fota_run_locked(prv_fota_start);
}
