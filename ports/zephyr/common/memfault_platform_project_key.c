//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Defines g_mflt_http_client_config and manages the Memfault project key.
//! This file is always compiled so the project key is available regardless of
//! whether HTTP, CoAP, BLE MDS, or MCUmgr transports are in use.

// clang-format off
#include <errno.h>
#include <string.h>
#include "memfault/core/compiler.h"
#include "memfault/core/debug_log.h"

#if defined(CONFIG_MEMFAULT_HTTP_CLIENT_CONFIG_BUILTIN)
  #include "memfault/http/http_client.h"
#endif

#if defined(CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS)
#include MEMFAULT_ZEPHYR_INCLUDE(settings/settings.h)
#endif
// clang-format on

#if defined(CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS)

static char s_mflt_project_key[MEMFAULT_PROJECT_KEY_LEN + 1];

static int prv_project_key_settings_set(const char *key, size_t len, settings_read_cb read_cb,
                                        void *cb_arg) {
  if (strcmp(key, "project_key") == 0) {
    ssize_t ret = read_cb(cb_arg, s_mflt_project_key, sizeof(s_mflt_project_key) - 1);
    if (ret < 0) {
      MEMFAULT_LOG_ERROR("Failed to read Memfault project key: %d", (int)ret);
      return (int)ret;
    }
    if (ret != MEMFAULT_PROJECT_KEY_LEN) {
      MEMFAULT_LOG_WARN("Memfault project key length mismatch: expected %d, got %d",
                        MEMFAULT_PROJECT_KEY_LEN, (int)ret);
    }
    s_mflt_project_key[ret] = '\0';
    MEMFAULT_LOG_DEBUG("Memfault project key loaded");
  }
  return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(memfault, "memfault", NULL, prv_project_key_settings_set, NULL,
                               NULL);

int memfault_zephyr_port_set_project_key(const char *key, size_t key_len) {
  if (key == NULL || key_len == 0 || key_len > MEMFAULT_PROJECT_KEY_LEN) {
    return -EINVAL;
  }
  memcpy(s_mflt_project_key, key, key_len);
  s_mflt_project_key[key_len] = '\0';
  return settings_save_one("memfault/project_key", key, key_len);
}

#endif /* CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS */

// CONFIG_MEMFAULT_NCS_PROJECT_KEY is the legacy nRF Connect SDK project key config (deprecated,
// use CONFIG_MEMFAULT_PROJECT_KEY). When set, it takes precedence for backward compatibility.
#if !defined(CONFIG_MEMFAULT_NCS_PROJECT_KEY) && !defined(CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS) && \
  (defined(CONFIG_BT_MDS) || defined(CONFIG_MEMFAULT_HTTP_ENABLE))
MEMFAULT_STATIC_ASSERT(
  sizeof(CONFIG_MEMFAULT_PROJECT_KEY) > 1,
  "Memfault Project Key not configured. Please visit https://mflt.io/project-key "
  "and add CONFIG_MEMFAULT_PROJECT_KEY=\"YOUR_KEY\" to prj.conf");
#endif

#if defined(CONFIG_MEMFAULT_HTTP_CLIENT_CONFIG_BUILTIN)
sMfltHttpClientConfig g_mflt_http_client_config = {
  #if defined(CONFIG_MEMFAULT_PROJECT_KEY_SETTINGS)
  .api_key = s_mflt_project_key,
  #elif defined(CONFIG_MEMFAULT_NCS_PROJECT_KEY)
  .api_key = CONFIG_MEMFAULT_NCS_PROJECT_KEY,
  #else
  .api_key = CONFIG_MEMFAULT_PROJECT_KEY,
  #endif
  #if defined(CONFIG_MEMFAULT_HTTP_DISABLE_TLS)
  .disable_tls = true,
  .chunks_api = {
    .host = sizeof(CONFIG_MEMFAULT_HTTP_CHUNKS_API_HOST) > 1 ?
              CONFIG_MEMFAULT_HTTP_CHUNKS_API_HOST : NULL,
    .port = 80,
  },
  .device_api = {
    .host = sizeof(CONFIG_MEMFAULT_HTTP_DEVICE_API_HOST) > 1 ?
              CONFIG_MEMFAULT_HTTP_DEVICE_API_HOST : NULL,
    .port = 80,
  },
  #endif
};
#endif  // defined(CONFIG_MEMFAULT_HTTP_CLIENT_CONFIG_BUILTIN)
