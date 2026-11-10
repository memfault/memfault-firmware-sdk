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
#include "memfault/nrfconnect_port/fota_component.h"
#include "memfault/ports/zephyr/fota.h"

#include "net/nrf_cloud.h"
#include "net/nrf_cloud_coap.h"
#include "net/nrf_cloud_fota_poll.h"

#include "memfault_fota_component_private.h"
// clang-format on

// nrf_cloud_fota_poll_ctx is captured at init time so that it can be used by
// memfault_zephyr_fota_start() and memfault_zephyr_fota_component_start() to trigger
// FOTA polls via Memfault SDK APIs (e.g. from the CLI)
static struct nrf_cloud_fota_poll_ctx *s_fota_ctx = NULL;

// Set by memfault_zephyr_fota_app_start() to make the next __wrap_nrf_cloud_coap_fota_job_get()
// call skip the component checks when no app update is available.
static bool s_app_only_requested = false;

//! Takes ownership of @p url and releases it via memfault_zephyr_port_coap_release_download_url()
//! before returning. The caller must not access @p url after this call.
static int prv_build_job_from_url(char *url, enum nrf_cloud_fota_type fota_type,
                                  const char *job_name,
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
  strncpy(job_id, job_name, NRF_CLOUD_FOTA_JOB_ID_SIZE - 1);
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

// When set, __wrap_nrf_cloud_coap_fota_job_get returns this job directly, allowing
// memfault_zephyr_fota_component_start() to skip the app update check entirely.
static struct nrf_cloud_fota_job_info s_prefetched_job = { .type = NRF_CLOUD_FOTA_TYPE__INVALID };

static enum nrf_cloud_fota_type prv_nrf_cloud_fota_type(eMemfaultFotaComponentInstall install) {
  switch (install) {
    case kMemfaultFotaComponentInstall_ModemDelta:
      return NRF_CLOUD_FOTA_MODEM_DELTA;
    case kMemfaultFotaComponentInstall_ModemFull:
      return NRF_CLOUD_FOTA_MODEM_FULL;
    case kMemfaultFotaComponentInstall_Bootloader:
      return NRF_CLOUD_FOTA_BOOTLOADER;
    default:
      return NRF_CLOUD_FOTA_TYPE__INVALID;
  }
}

//! @return 1 if *job_out was filled with an update job, 0 if no update is pending, <0 on error
static int prv_check_component_update(const sMemfaultFotaComponent *component,
                                      struct nrf_cloud_fota_job_info *const job_out) {
  *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };

  if (component->install == kMemfaultFotaComponentInstall_None) {
    // Installed by the application, see
    // memfault_zephyr_fota_component_get_download_url()
    return -ENOTSUP;
  }

  const enum nrf_cloud_fota_type fota_type = prv_nrf_cloud_fota_type(component->install);
  if (fota_type == NRF_CLOUD_FOTA_TYPE__INVALID) {
    MEMFAULT_LOG_ERROR("'%s' has an invalid install value: %d", component->software_type,
                       (int)component->install);
    return -EINVAL;
  }

  char *url = NULL;
  int rv = memfault_fota_component_url_get(component, &url);

  if (rv <= 0) {
    if (rv == 0) {
      MEMFAULT_LOG_DEBUG("No pending '%s' FOTA update", component->software_type);
    }
    return rv;
  }

  MEMFAULT_LOG_DEBUG("'%s' FOTA update available. URL: %s", component->software_type, url);
  rv = prv_build_job_from_url(url, fota_type, component->software_type, job_out);
  return (rv < 0) ? rv : 1;
}

static int prv_check_component_update_cb(const sMemfaultFotaComponent *component, void *ctx) {
  return prv_check_component_update(component, (struct nrf_cloud_fota_job_info *)ctx);
}

// Checks every registered component, filling *job_out with the first one with an update.
static int prv_check_components(struct nrf_cloud_fota_job_info *const job_out) {
  *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
  int rv = memfault_fota_component_updates_check(prv_check_component_update_cb, job_out);
  // nrf_cloud_coap_fota_job_get() contract: 0 with job_out->type set when a job is available
  return (rv > 0) ? 0 : rv;
}

//! @param app_only Skip the component checks when no app update is available, instead of
//! falling through to them. Used by memfault_zephyr_fota_app_start() to guarantee an app-only
//! check.
static int prv_perform_fota_check(struct nrf_cloud_fota_job_info *const job_out, bool app_only) {
  MEMFAULT_LOG_DEBUG("Checking Memfault for FOTA update");

  char *url = NULL;
  int rv = memfault_zephyr_port_coap_get_download_url(&url);

  if (rv < 0) {
    return rv;
  }

  if (rv > 0) {
    MEMFAULT_LOG_DEBUG("App FOTA update available. URL: %s", url);
    return prv_build_job_from_url(url, NRF_CLOUD_FOTA_APPLICATION, "APP", job_out);
  }

  // No app update available: check the components
  if (!app_only) {
    return prv_check_components(job_out);
  }
  MEMFAULT_LOG_DEBUG("No pending FOTA update");
  *job_out = (struct nrf_cloud_fota_job_info){ .type = NRF_CLOUD_FOTA_TYPE__INVALID };
  return 0;
}

int __real_nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job);
int __wrap_nrf_cloud_coap_fota_job_get(struct nrf_cloud_fota_job_info *const job) {
  bool app_only = s_app_only_requested;
  s_app_only_requested = false;

  // An app-only request takes precedence over a stale pre-fetched job (e.g. left over from a
  // memfault_zephyr_fota_component_start() poll that failed before consuming it).
  if (app_only) {
    s_prefetched_job.type = NRF_CLOUD_FOTA_TYPE__INVALID;
  } else if (s_prefetched_job.type != NRF_CLOUD_FOTA_TYPE__INVALID) {
    // Skip the application check and return the pre-fetched component job.
    *job = s_prefetched_job;
    s_prefetched_job.type = NRF_CLOUD_FOTA_TYPE__INVALID;  // clear the job
    return 0;
  }
  return prv_perform_fota_check(job, app_only);
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

int memfault_zephyr_fota_app_start(void) {
  if (!s_fota_ctx) {
    MEMFAULT_LOG_ERROR("nrf_cloud_fota_poll_init() has not been called yet; "
                       "no ctx available to trigger a poll");
    return -EINVAL;
  }
  // Skip the component checks in __wrap_nrf_cloud_coap_fota_job_get() if no app update is
  // available.
  s_app_only_requested = true;
  int rv = nrf_cloud_fota_poll_process(s_fota_ctx);
  // Clear the request here too, in case the poll returned without ever invoking the wrapped job
  // getter (e.g. a poll already in progress), so it cannot leak into a later unrelated check.
  s_app_only_requested = false;
  return (rv == -EAGAIN) ? 0 : rv;
}

int memfault_zephyr_fota_component_start(const char *software_type) {
  if (!s_fota_ctx) {
    MEMFAULT_LOG_ERROR("nrf_cloud_fota_poll_init() has not been called yet; "
                       "no ctx available to trigger a poll");
    return -EINVAL;
  }
  const sMemfaultFotaComponent *component = memfault_fota_component_find(software_type);
  if (!component) {
    return -ENOENT;
  }
  // Pre-fetch the job so __wrap_nrf_cloud_coap_fota_job_get returns it directly,
  // and skips the app update check when nrf_cloud_fota_poll_process() is called below.
  int rv = prv_check_component_update(component, &s_prefetched_job);
  if (rv <= 0) {
    return rv;
  }
  rv = nrf_cloud_fota_poll_process(s_fota_ctx);
  return (rv == -EAGAIN) ? 0 : rv;
}

int memfault_zephyr_fota_component_get_download_url(const char *software_type, char **url) {
  const sMemfaultFotaComponent *component = memfault_fota_component_find(software_type);
  return component ? memfault_fota_component_url_get(component, url) : -ENOENT;
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
