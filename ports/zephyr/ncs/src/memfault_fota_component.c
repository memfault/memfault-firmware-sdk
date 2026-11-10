//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief FOTA component registry, used by the nRF Connect SDK and nRF Cloud override FOTA
//! backends. See memfault/nrfconnect_port/fota_component.h.

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#include <errno.h>
#include <string.h>

#include "memfault/components.h"
#include "memfault/nrfconnect_port/fota_component.h"
#include "memfault_fota_component_private.h"

#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  #include "memfault/nrfconnect_port/coap.h"
#else
  #include "memfault/ports/zephyr/http.h"
#endif

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
  #include <modem/modem_info.h>

  #include "memfault_fota_modem_project_key_private.h"
#endif
// clang-format on

// The component being queried and its current version, read by the get_device_info callback
// (which takes no context argument). Guarded by s_memfault_fota_component_mutex.
static K_MUTEX_DEFINE(s_memfault_fota_component_mutex);
static const sMemfaultFotaComponent *s_memfault_fota_component_active;
static char s_memfault_fota_component_version[64];

static void prv_fota_component_device_info_get(sMemfaultDeviceInfo *info) {
  memfault_platform_get_device_info(info);
  const sMemfaultFotaComponent *component = s_memfault_fota_component_active;
  if (component == NULL) {
    // Not in a component query: report the main application
    return;
  }
  info->software_type = component->software_type;
  info->software_version = s_memfault_fota_component_version;
}

const sMemfaultFotaComponent *memfault_fota_component_find(const char *software_type) {
  STRUCT_SECTION_FOREACH(MemfaultFotaComponent, component) {
    if (strcmp(component->software_type, software_type) == 0) {
      return component;
    }
  }
  return NULL;
}

// Must be called with s_memfault_fota_component_mutex held
static int prv_fota_component_url_get_locked(const sMemfaultFotaComponent *component, char **url) {
  int rv = component->version_get(s_memfault_fota_component_version,
                                  sizeof(s_memfault_fota_component_version));
  if (rv < 0) {
    return rv;
  }
  MEMFAULT_LOG_DEBUG("Checking for '%s' FOTA (current version: %s)", component->software_type,
                     s_memfault_fota_component_version);

  // Swap the component's software type, version and project key into the HTTP client config for
  // this query only. CoAP also reads the project key from it.
  const char *component_project_key =
    component->project_key_get ? component->project_key_get() : NULL;

  const char *saved_api_key = g_mflt_http_client_config.api_key;
  void (*saved_get_device_info)(sMemfaultDeviceInfo *) = g_mflt_http_client_config.get_device_info;

  if (component_project_key) {
    g_mflt_http_client_config.api_key = component_project_key;
  }
  s_memfault_fota_component_active = component;
  g_mflt_http_client_config.get_device_info = prv_fota_component_device_info_get;

#if defined(CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP)
  rv = memfault_zephyr_port_coap_get_download_url(url);
#else
  rv = memfault_zephyr_port_get_download_url(url);
#endif

  g_mflt_http_client_config.api_key = saved_api_key;
  g_mflt_http_client_config.get_device_info = saved_get_device_info;
  s_memfault_fota_component_active = NULL;

  return rv;
}

int memfault_fota_component_url_get(const sMemfaultFotaComponent *component, char **url) {
  k_mutex_lock(&s_memfault_fota_component_mutex, K_FOREVER);
  const int rv = prv_fota_component_url_get_locked(component, url);
  k_mutex_unlock(&s_memfault_fota_component_mutex);
  return rv;
}

int memfault_fota_component_updates_check(int (*check)(const sMemfaultFotaComponent *component,
                                                       void *ctx),
                                          void *ctx) {
  STRUCT_SECTION_FOREACH(MemfaultFotaComponent, component) {
    if (component->install == kMemfaultFotaComponentInstall_None) {
      continue;
    }
    int rv = check(component, ctx);
    if (rv > 0) {
      return rv;
    }
    if (rv == -ETIMEDOUT || rv == -ENOTCONN) {
      return rv;
    }
    if (rv < 0 && rv != -ENOTSUP) {
      MEMFAULT_LOG_WARN("Check of '%s' failed (%d), continuing with other components",
                        component->software_type, rv);
    }
  }
  return 0;
}

#if defined(CONFIG_MEMFAULT_FOTA_MODEM_UPDATE)
static int prv_fota_component_modem_version_get(char *buf, size_t buf_len) {
  int rv = modem_info_string_get(MODEM_INFO_FW_VERSION, buf, buf_len);
  return (rv < 0) ? rv : 0;
}

// The modem is a built-in component. A dedicated modem project key, if set, takes precedence over
// the main project. With CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE=y, every modem download is
// prepared as a possible full image; the DFU target detects delta vs. full from the image header.
MEMFAULT_FOTA_COMPONENT_DEFINE_WITH_PROJECT_KEY(memfault_modem,
                                                CONFIG_MEMFAULT_FOTA_MODEM_SOFTWARE_TYPE,
                                                prv_fota_component_modem_version_get,
                                                IS_ENABLED(CONFIG_MEMFAULT_FOTA_FULL_MODEM_UPDATE) ?
                                                  kMemfaultFotaComponentInstall_ModemFull :
                                                  kMemfaultFotaComponentInstall_ModemDelta,
                                                memfault_zephyr_fota_modem_project_key_get);
#endif  // CONFIG_MEMFAULT_FOTA_MODEM_UPDATE
