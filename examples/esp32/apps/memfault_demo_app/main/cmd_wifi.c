//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Wifi-specific commands for the ESP32 console.

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "settings.h"

// enable for more verbose debug logs
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

// max length of saved ssid / pw, including null term
#define WIFI_CREDS_MAX_SIZE 65

// cache the creds so we don't have to read NVS every time they're requested
static char s_wifi_ssid[WIFI_CREDS_MAX_SIZE];
static char s_wifi_pass[WIFI_CREDS_MAX_SIZE];

#define EXAMPLE_ESP_MAXIMUM_RETRY 10 // CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#else
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGD(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGD(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

bool wifi_join(const char *ssid, const char *pass) {
  static bool connected_successfully = false;
  static bool one_time_init = false;
  if (!one_time_init) {
    one_time_init = true;

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    // Try to create default event loop. If it's already created, it returns
    // ESP_ERR_INVALID_STATE, equivalent to ESP_OK for our purposes.
    esp_err_t err = esp_event_loop_create_default();
    ESP_ERROR_CHECK_WITHOUT_ABORT(err == ESP_ERR_INVALID_STATE ? ESP_OK : err);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL, &instance_got_ip));
  }
  wifi_config_t wifi_config = {
      .sta =
          {
              /* Setting a password implies station will connect to all security
               * modes including WEP/WPA. However these modes are deprecated and
               * not advisable to be used. Incase your Access point doesn't
               * support WPA2, these mode can be enabled by commenting out the line
               * below */
              .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
          },
  };
  if (connected_successfully) {
    // reconnect attempt
    ESP_LOGD(TAG, "wifi already connected, reconnecting ...");
    esp_wifi_disconnect();
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
      return false;
    }
  } else {
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGD(TAG, "wifi_init_sta finished.");
  }
  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s ", ssid);
    connected_successfully = true;
    return true;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGD(TAG, "Failed to connect to SSID:%s", pass);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  return false;
}

static int wifi_disconnect() {
  return esp_wifi_disconnect();
}

/** Arguments used by 'join' function */
static struct {
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_end *end;
} join_args;

static int connect(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&join_args);
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
  // length arg is ignored for string-type settings
  esp_err_t err = settings_set(kSettingsWifiSsid, ssid, 0);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) writing ssid to NVS!", esp_err_to_name(err));
  }
  err = settings_set(kSettingsWifiPassword, password, 0);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) writing password to NVS!", esp_err_to_name(err));
  }
  if (err == ESP_OK) {
    ESP_LOGD(__func__, "Successfully saved new wifi creds");
    strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
    strncpy(s_wifi_pass, password, sizeof(s_wifi_pass) - 1);
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

  ESP_LOGD(__func__, "Reading wifi creds ... ");
  size_t len = sizeof(s_wifi_ssid);
  esp_err_t err = settings_get(kSettingsWifiSsid, s_wifi_ssid, &len);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "failed reading ssid");
    return;
  }

  len = sizeof(s_wifi_pass);
  err = settings_get(kSettingsWifiPassword, s_wifi_pass, &len);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "failed reading pw");
    return;
  }

  *ssid = s_wifi_ssid;
  *password = s_wifi_pass;
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

static struct {
  struct arg_str* projectkey;
  struct arg_end* end;
} memfault_args;

__attribute__((access(write_only, 1, 2))) int wifi_get_project_key(char* project_key,
                                                                   size_t project_key_len) {
  if (project_key_len != MEMFAULT_PROJECT_KEY_LEN + 1) {
    ESP_LOGE(__func__, "Destination buffer must be sized exactly to %zu bytes",
             MEMFAULT_PROJECT_KEY_LEN + 1);
    return 1;
  }
  size_t len = project_key_len;
  return settings_get(kSettingsProjectKey, project_key, &len);
}

__attribute__((access(read_only, 1, 2))) static esp_err_t prv_set_project_key(
  const char* project_key, size_t project_key_len) {
  // should never happen
  assert((project_key_len == MEMFAULT_PROJECT_KEY_LEN) &&
         (strlen(project_key) == MEMFAULT_PROJECT_KEY_LEN));

  return settings_set(kSettingsProjectKey, project_key, project_key_len);
}

static int project_key_set(int argc, char** argv) {
  if (argc == 1) {
    char project_key[MEMFAULT_PROJECT_KEY_LEN + 1];
    esp_err_t err = wifi_get_project_key(project_key, sizeof(project_key));
    if (err != ESP_OK) {
      ESP_LOGE(__func__, "failed to load wifi creds");
      return 1;
    }

    printf("%.*s\n", MEMFAULT_PROJECT_KEY_LEN, project_key);
    return 0;
  }

  int nerrors = arg_parse(argc, argv, (void**)&memfault_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, memfault_args.end, argv[0]);
    return 1;
  }

  // set the project key to nvs
  const char* projectkey = memfault_args.projectkey->sval[0];
  if (strlen(projectkey) != MEMFAULT_PROJECT_KEY_LEN) {
    ESP_LOGE(__func__, "Project key must be %d characters", MEMFAULT_PROJECT_KEY_LEN);
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t err = prv_set_project_key(projectkey, strlen(projectkey));

  return err;
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

  memfault_args.projectkey = arg_str1(NULL, NULL, "<project_key>", "Memfault Project Key");
  memfault_args.end = arg_end(1);
  const esp_console_cmd_t project_key_cmd = {.command = "project_key",
                                             .help = "Save Memfault Project Key to NVS",
                                             .hint = NULL,
                                             .func = &project_key_set,
                                             .argtable = &memfault_args};
  ESP_ERROR_CHECK(esp_console_cmd_register(&project_key_cmd));

  const esp_console_cmd_t disconnect_cmd = {.command = "wifi_disconnect",
                                            .help = "Disconnect from WiFi AP",
                                            .hint = NULL,
                                            .func = &wifi_disconnect,
                                            .argtable = NULL};
  ESP_ERROR_CHECK(esp_console_cmd_register(&disconnect_cmd));
}
