//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Version of cmd_wifi.c that uses the legacy wifi API, for ESP-IDF v3.

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "esp_console.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

// enable for more verbose debug logs
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

// max length of saved ssid / pw, including null term
#define WIFI_CREDS_MAX_SIZE 65

// cache the creds so we don't have to read NVS every time they're requested
static char s_wifi_ssid[WIFI_CREDS_MAX_SIZE];
static char s_wifi_pass[WIFI_CREDS_MAX_SIZE];

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// this type changed in ESP-IDF v4.0, to 'nvs_handle_t'; add a backwards-compat
// typedef
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
typedef nvs_handle nvs_handle_t;
#endif

int wifi_get_project_key(char* project_key, size_t project_key_len) {
  // Configurable project key not supported, project key must be compiled in via
  // CONFIG_MEMFAULT_PROJECT_KEY
  return 1;
}

static esp_err_t event_handler(void* ctx, system_event_t* event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
      xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
      break;
    default:
      break;
  }
  return ESP_OK;
}

static void initialise_wifi(void) {
  esp_log_level_set("wifi", ESP_LOG_WARN);
  static bool initialized = false;
  if (initialized) {
    return;
  }
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  initialized = true;
}

bool wifi_join(const char* ssid, const char* pass) {
  initialise_wifi();
  wifi_config_t wifi_config = {0};
  strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (pass) {
    strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_connect());

  const int timeout_ms = 5000;
  int bits =
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 1, 1, timeout_ms / portTICK_PERIOD_MS);
  return (bits & CONNECTED_BIT) != 0;
}

/** Arguments used by 'join' function */
static struct {
  struct arg_str* ssid;
  struct arg_str* password;
  struct arg_end* end;
} join_args;

static int connect(int argc, char** argv) {
  int nerrors = arg_parse(argc, argv, (void**)&join_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, join_args.end, argv[0]);
    return 1;
  }
  ESP_LOGI(__func__, "Connecting to '%s'", join_args.ssid->sval[0]);

  bool connected =
    wifi_join(join_args.ssid->sval[0], join_args.password->sval[0]);
  if (!connected) {
    ESP_LOGW(__func__, "Connection timed out");
    return 1;
  }
  ESP_LOGI(__func__, "Connected");
  return 0;
}

//! Since the creds are set via cli, we can safely assume there's no null bytes
//! in the password, despite being theoretically permissible by the spec
static void prv_save_wifi_creds(const char* ssid, const char* password) {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open
  nvs_handle_t nvs_handle;
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGD(__func__, "Opened NVS handle");

    // Write
    err = nvs_set_str(nvs_handle, "wifi_ssid", ssid);
    if (err != ESP_OK) {
      ESP_LOGE(__func__, "Error (%s) writing ssid to NVS!", esp_err_to_name(err));
    } else {
      ESP_LOGD(__func__, "Wrote ssid to NVS");
    }
    err = nvs_set_str(nvs_handle, "wifi_password", password);
    if (err == ESP_OK) {
      if (err != ESP_OK) {
        ESP_LOGE(__func__, "Error (%s) writing password to NVS!", esp_err_to_name(err));
      } else {
        ESP_LOGD(__func__, "Wrote password to NVS");
      }
    }

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
      ESP_LOGE(__func__, "Error (%s) committing NVS!", esp_err_to_name(err));
    } else {
      ESP_LOGD(__func__, "Successfully saved new wifi creds");
      strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid));
      strncpy(s_wifi_pass, password, sizeof(s_wifi_pass));
    }

    // Close
    nvs_close(nvs_handle);
  }
}

//! Return ssid + password pointers, or NULL if not found
void wifi_load_creds(char** ssid, char** password) {
  // first check if the creds are already loaded
  if ((strlen(s_wifi_ssid) > 0) && (strlen(s_wifi_pass) > 0)) {
    *ssid = s_wifi_ssid;
    *password = s_wifi_pass;
    return;
  }

  // clear output first
  *ssid = NULL;
  *password = NULL;

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open
  nvs_handle_t nvs_handle;
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    ESP_LOGD(__func__, "Opened NVS handle");

    // Read
    ESP_LOGD(__func__, "Reading wifi creds ... ");
    size_t len = sizeof(s_wifi_ssid);
    err = nvs_get_str(nvs_handle, "wifi_ssid", s_wifi_ssid, &len);
    if (err == ESP_OK) {
      ESP_LOGD(__func__, "ssid size: %d", len);
    } else {
      ESP_LOGE(__func__, "failed reading ssid");
      goto close;
    }

    len = sizeof(s_wifi_pass);
    err = nvs_get_str(nvs_handle, "wifi_password", s_wifi_pass, &len);
    if (err == ESP_OK) {
      ESP_LOGD(__func__, "pw size: %d", len);
    } else {
      ESP_LOGE(__func__, "failed reading pw");
      goto close;
    }

    *ssid = s_wifi_ssid;
    *password = s_wifi_pass;

  close:
    nvs_close(nvs_handle);
  }
}

// argtable3 requires this data structure to be valid through the entire command
// session, so we can't just use a local variable.
static struct {
  struct arg_str* ssid;
  struct arg_str* password;
  struct arg_end* end;
} wifi_creds_args;

static int wifi_creds_set(int argc, char** argv) {
  if (argc == 1) {
    char* ssid;
    char* password;
    wifi_load_creds(&ssid, &password);
    if (!ssid || !password) {
      ESP_LOGE(__func__, "failed to load wifi creds");
      return 1;
    }

    printf("ssid: %s pw: %s\n", ssid, password);
    return 0;
  }
  int nerrors = arg_parse(argc, argv, (void**)&wifi_creds_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, wifi_creds_args.end, argv[0]);
    return 1;
  }

  const char* ssid = wifi_creds_args.ssid->sval[0];
  if (strlen(ssid) > 32) {
    ESP_LOGE(__func__, "SSID must be 32 characters or less");
    return 1;
  }

  const char* password = wifi_creds_args.password->sval[0];
  if (strlen(password) > 64) {
    ESP_LOGE(__func__, "Password must be 64 characters or less");
    return 1;
  }

  prv_save_wifi_creds(ssid, password);

  return 0;
}

void register_wifi(void) {
  join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
  join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
  join_args.end = arg_end(2);

  const esp_console_cmd_t join_cmd = {.command = "join",
                                      .help = "Join WiFi AP as a station",
                                      .hint = NULL,
                                      .func = &connect,
                                      .argtable = &join_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));

  wifi_creds_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
  wifi_creds_args.password = arg_str1(NULL, NULL, "<pass>", "PSK of AP");
  wifi_creds_args.end = arg_end(2);
  const esp_console_cmd_t config_cmd = {.command = "wifi_config",
                                        .help = "Save WiFi ssid + password to NVS",
                                        .hint = NULL,
                                        .func = &wifi_creds_set,
                                        .argtable = &wifi_creds_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
}
