//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#include <errno.h>
#include <string.h>

#include "memfault/components.h"
#include "memfault/nrfconnect_port/coap.h"
#include "memfault/ports/zephyr/fota.h"

#include "net/nrf_cloud.h"
#include "net/nrf_cloud_coap.h"
#include "net/nrf_cloud_fota_poll.h"

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  #include <modem/modem_info.h>

  #include "memfault_fota_modem_project_key_private.h"
#endif
// clang-format on

// nrf_cloud_fota_poll_ctx is captured at init time so that it can be used by
// memfault_zephyr_fota_start() and memfault_zephyr_fota_modem_start() to trigger FOTA polls via
// Memfault SDK APIs (e.g. from the CLI)
static struct nrf_cloud_fota_poll_ctx *s_fota_ctx = NULL;

//! Takes ownership of @p url and releases it via memfault_zephyr_port_coap_release_download_url()
//! before returning. The caller must not access @p url after this call.
static int prv_build_job_from_url(char *url, enum nrf_cloud_fota_type fota_type,
                                  struct nrf_cloud_fota_job_info *const job_out) {
  // Split the URL into host and file components, same pattern as memfault_fota.c.
  // Skip past the scheme (e.g. "https://") before searching for the path separator ('/').
  char *host_start = strstr(url, "://");
  host_start = host_start ? host_start + 3 : url;
  char *file = strchr(host_start, '/');
  if (!file) {
    MEMFAULT_LOG_ERROR("Invalid URL format");
    memfault_zephyr_port_coap_release_download_url(&url);
    return -1;
  }

  // Insert a null terminator to split host and file, so we can copy each substring.
  *file = '\0';
  file++;  // file now points to the path after the first '/'

  MEMFAULT_LOG_DEBUG("Split URL - host: '%s', file: '%s'", host_start, file);

  // Use k_malloc so the allocations are compatible with nrf_cloud_coap_fota_job_free(),
  // which frees host, path, and id via k_free().
  char *host = k_malloc(strlen(host_start) + 1);
  char *path = k_malloc(strlen(file) + 1);

  // nrf_cloud_fota_poll does memcpy(pending_job.id, job.id, NRF_CLOUD_FOTA_JOB_ID_SIZE) before
  // reboot, so the buffer must be NRF_CLOUD_FOTA_JOB_ID_SIZE bytes. Use k_calloc to zero the
  // tail so the fixed-size memcpy copies deterministic bytes rather than heap garbage.
  char *job_id = k_calloc(1, NRF_CLOUD_FOTA_JOB_ID_SIZE);

  if (!host || !path || !job_id) {
    k_free(host);
    k_free(path);
    k_free(job_id);
    memfault_zephyr_port_coap_release_download_url(&url);
    return -ENOMEM;
  }

  // nrf_cloud_coap_transport_proxy_dl_uri_get() prepends "https://" to the host, so pass the
  // bare hostname here, not the full URL which still carries the scheme.
  strcpy(host, host_start);
  strcpy(path, file);
  strcpy(job_id, (fota_type == NRF_CLOUD_FOTA_APPLICATION) ? "APP" : "MODEM");
  memfault_zephyr_port_coap_release_download_url(&url);

  *job_out = (struct nrf_cloud_fota_job_info){
    .type = fota_type,
    .id = job_id,
    .host = host,
    .path = path,
    .file_size = 0,  // unused by nrf_cloud_download_start()
  };
  return 0;
}

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)

// When set, __wrap_nrf_cloud_coap_fota_job_get returns this job directly, allowing
// memfault_zephyr_fota_modem_start() to skip the app update check entirely.
static struct nrf_cloud_fota_job_info s_prefetched_modem_job = { .type =
                                                                   NRF_CLOUD_FOTA_TYPE__INVALID };

// Buffer for the running modem firmware version string (e.g. "mfw_nrf9160_1.3.7")
static char s_modem_version[64];

static void prv_get_modem_device_info(sMemfaultDeviceInfo *info) {
  memfault_platform_get_device_info(info);
  info->software_type = CONFIG_MEMFAULT_FOTA_MODEM_SOFTWARE_TYPE;
  info->software_version = s_modem_version;
}

static int prv_check_modem_update(struct nrf_cloud_fota_job_info *const job_out) {
  // Populate modem version once per boot - the version never changes between reboots.
  if (s_modem_version[0] == '\0') {
    int rv = modem_info_string_get(MODEM_INFO_FW_VERSION, s_modem_version, sizeof(s_modem_version));
    if (rv < 0) {
      MEMFAULT_LOG_ERROR("Failed to read modem FW version: %d", rv);
      *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
      return rv;
    }
  }

  // Temporarily switch the HTTP client config to use the modem project credentials for the URL
  // query. The key is used only while building/sending the request below and is restored
  // immediately after, so concurrent data uploads are not a concern in practice - FOTA and
  // upload both run on the periodic upload thread and are serialized there.
  const char *modem_project_key = memfault_zephyr_fota_modem_project_key_get();
  if (!modem_project_key) {
    MEMFAULT_LOG_WARN("Modem project key not set. Set CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY "
                      "or call memfault_zephyr_fota_modem_project_key_set().");
    *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
    return 0;
  }

  const char *saved_api_key = g_mflt_http_client_config.api_key;
  void (*saved_get_device_info)(sMemfaultDeviceInfo *) = g_mflt_http_client_config.get_device_info;

  g_mflt_http_client_config.api_key = modem_project_key;
  g_mflt_http_client_config.get_device_info = prv_get_modem_device_info;

  // Query the modem update URL over nRF Cloud CoAP. The modem project key (set above) is
  // injected as CoAP option 2429 by nrf_cloud_coap_get_user_options(), and the overridden
  // get_device_info supplies the modem software type/version used to build the OTA URL.
  char *url = NULL;
  int rv = memfault_zephyr_port_coap_get_download_url(&url);

  g_mflt_http_client_config.api_key = saved_api_key;
  g_mflt_http_client_config.get_device_info = saved_get_device_info;

  if (rv <= 0) {
    if (rv == 0) {
      MEMFAULT_LOG_DEBUG("No pending modem FOTA update");
    }
    *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
    return rv;
  }

  MEMFAULT_LOG_DEBUG("Modem FOTA update available. URL: %s", url);
  return prv_build_job_from_url(url, NRF_CLOUD_FOTA_MODEM_DELTA, job_out);
}

#endif  // CONFIG_MEMFAULT_FOTA_MODEM_UPDATE

static int prv_perform_fota_check(struct nrf_cloud_fota_job_info *const job_out) {
  MEMFAULT_LOG_DEBUG("Checking Memfault for FOTA update");

  char *url = NULL;
  int rv = memfault_zephyr_port_coap_get_download_url(&url);

  if (rv < 0) {
    return rv;
  }

  if (rv > 0) {
    MEMFAULT_LOG_DEBUG("App FOTA update available. URL: %s", url);
    return prv_build_job_from_url(url, NRF_CLOUD_FOTA_APPLICATION, job_out);
  }

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  // Check for modem update if no app update is available
  return prv_check_modem_update(job_out);
#else
  MEMFAULT_LOG_DEBUG("No pending FOTA update");
  *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
  return 0;
#endif
}

int __real_nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job);
int __wrap_nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job) {
#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  // Skip application FOTA check and return the pre-fetched modem job if available.
  // This path supports triggering modem FOTA only via memfault_zephyr_fota_modem_start()
  if (s_prefetched_modem_job.type != NRF_CLOUD_FOTA_TYPE__INVALID) {
    *job = s_prefetched_modem_job;
    s_prefetched_modem_job.type = NRF_CLOUD_FOTA_TYPE__INVALID;  // clear the job
    return 0;
  }
#endif
  return prv_perform_fota_check(job);
}

_Static_assert(__builtin_types_compatible_p(__typeof__(&nrf_cloud_coap_fota_job_get),
                                            __typeof__(&__wrap_nrf_cloud_coap_fota_job_get)),
               "Wrapped function signature mismatch: nrf_cloud_coap_fota_job_get");

// nRF Cloud FOTA reports the result of the FOTA back to nRF Cloud via
// nrf_cloud_coap_fota_job_update(). Since this operation does not currently have an equivalent in
// the Memfault OTA system, do nothing and return success unconditionally.
int __real_nrf_cloud_coap_fota_job_update(const char *const job_id,
                                          const enum nrf_cloud_fota_status status,
                                          const char *const details);
int __wrap_nrf_cloud_coap_fota_job_update(MEMFAULT_UNUSED const char *const job_id,
                                          MEMFAULT_UNUSED const enum nrf_cloud_fota_status status,
                                          MEMFAULT_UNUSED const char *const details) {
  MEMFAULT_LOG_DEBUG("Skipping nRF Cloud FOTA job update");
  return 0;
}

_Static_assert(__builtin_types_compatible_p(__typeof__(&nrf_cloud_coap_fota_job_update),
                                            __typeof__(&__wrap_nrf_cloud_coap_fota_job_update)),
               "Wrapped function signature mismatch: nrf_cloud_coap_fota_job_update");

// Wrap nrf_cloud_fota_poll_init to capture the ctx the application passes at init time.
// memfault_zephyr_fota_start() then calls nrf_cloud_fota_poll_process() with that ctx
// on demand (e.g. from the shell) without any separate registration call.
int __real_nrf_cloud_fota_poll_init(struct nrf_cloud_fota_poll_ctx *ctx);
int __wrap_nrf_cloud_fota_poll_init(struct nrf_cloud_fota_poll_ctx *ctx) {
  s_fota_ctx = ctx;
  return __real_nrf_cloud_fota_poll_init(ctx);
}

_Static_assert(__builtin_types_compatible_p(__typeof__(&nrf_cloud_fota_poll_init),
                                            __typeof__(&__wrap_nrf_cloud_fota_poll_init)),
               "Wrapped function signature mismatch: nrf_cloud_fota_poll_init");

int memfault_zephyr_fota_start(void) {
  if (!s_fota_ctx) {
    MEMFAULT_LOG_ERROR("nrf_cloud_fota_poll_init() has not been called yet; "
                       "no ctx available to trigger a poll");
    return -EINVAL;
  }
  int rv = nrf_cloud_fota_poll_process(s_fota_ctx);
  // -EAGAIN means the poll completed successfully with no pending job; normalise to 0
  return (rv == -EAGAIN) ? 0 : rv;
}

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
int memfault_zephyr_fota_modem_start(void) {
  if (!s_fota_ctx) {
    MEMFAULT_LOG_ERROR("nrf_cloud_fota_poll_init() has not been called yet; "
                       "no ctx available to trigger a poll");
    return -EINVAL;
  }
  // Pre-fetch the modem job so __wrap_nrf_cloud_coap_fota_job_get returns it directly,
  // and skips the app update check when nrf_cloud_fota_poll_process() is called below.
  int rv = prv_check_modem_update(&s_prefetched_modem_job);
  if (rv < 0 || s_prefetched_modem_job.type == NRF_CLOUD_FOTA_TYPE__INVALID) {
    return rv;
  }
  rv = nrf_cloud_fota_poll_process(s_fota_ctx);
  return (rv == -EAGAIN) ? 0 : rv;
}
#endif  // CONFIG_MEMFAULT_FOTA_MODEM_UPDATE
