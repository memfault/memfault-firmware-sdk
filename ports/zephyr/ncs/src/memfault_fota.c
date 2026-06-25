//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(shell/shell.h)

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
#endif

#include "memfault/components.h"

#include "memfault/ports/zephyr/root_cert_storage.h"

#include "net/downloader.h"
#include "net/fota_download.h"
// clang-format on

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  #include <modem/modem_info.h>

  #include "memfault_fota_modem_project_key_private.h"
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
    // intentional fall through, no action on other event ids
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
  }
  return rv;
}

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)

// Buffer for the running modem firmware version string (e.g. "mfw_nrf9160_1.3.7")
static char s_modem_version[64];

static void prv_get_modem_device_info(sMemfaultDeviceInfo *info) {
  memfault_platform_get_device_info(info);
  info->software_type = CONFIG_MEMFAULT_FOTA_MODEM_SOFTWARE_TYPE;
  info->software_version = s_modem_version;
}

static int prv_fota_modem_start(void) {
  int rv = modem_info_string_get(MODEM_INFO_FW_VERSION, s_modem_version, sizeof(s_modem_version));
  if (rv < 0) {
    MEMFAULT_LOG_ERROR("Failed to read modem FW version: %d", rv);
    return rv;
  }
  MEMFAULT_LOG_INFO("Checking modem FOTA (current version: %s)", s_modem_version);

  // Temporarily switch the HTTP client config to use the modem project credentials for the URL
  // query. The key is used only while building/sending the download-URL request below and is
  // restored immediately after, so concurrent data uploads are not a concern in practice - FOTA
  // and upload both run on the periodic upload thread and are serialized there.
  const char *modem_project_key = memfault_zephyr_fota_modem_project_key_get();
  if (!modem_project_key) {
    MEMFAULT_LOG_ERROR("Modem project key not set. Set CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY "
                       "or call memfault_zephyr_fota_modem_project_key_set().");
    return -EINVAL;
  }

  const char *saved_api_key = g_mflt_http_client_config.api_key;
  void (*saved_get_device_info)(sMemfaultDeviceInfo *) = g_mflt_http_client_config.get_device_info;

  g_mflt_http_client_config.api_key = modem_project_key;
  g_mflt_http_client_config.get_device_info = prv_get_modem_device_info;

  #if CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP
  // Query the modem update URL over nRF Cloud CoAP. The modem project key (set above) is injected
  // as CoAP option 2429 by nrf_cloud_coap_get_user_options(), and the overridden get_device_info
  // supplies the modem software type/version used to build the OTA URL.
  rv = memfault_zephyr_port_coap_get_download_url(&s_download_url);
  #else
  rv = memfault_zephyr_port_get_download_url(&s_download_url);
  #endif

  g_mflt_http_client_config.api_key = saved_api_key;
  g_mflt_http_client_config.get_device_info = saved_get_device_info;

  if (rv <= 0) {
    if (rv == 0) {
      MEMFAULT_LOG_INFO("Modem firmware is up to date");
    }
    return rv;
  }

  return prv_fota_download_start();
}

int memfault_zephyr_fota_modem_start(void) {
  return prv_fota_modem_start();
}

#endif  // CONFIG_MEMFAULT_FOTA_MODEM_UPDATE

int memfault_zephyr_fota_start(void) {
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

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  // No app update available - check for modem firmware update
  return prv_fota_modem_start();
#else
  return 0;
#endif
}
