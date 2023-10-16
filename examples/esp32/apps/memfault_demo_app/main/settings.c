//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Implement app settings helpers

#include "settings.h"

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "esp_console.h"
#include "memfault/components.h"
#include "nvs.h"
#include "nvs_flash.h"

// enable for more verbose debug logs
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

//! settings types: string or int
enum settings_type {
  kSettingsTypeString,
  kSettingsTypeI32,
};

//! settings key to {type, key_string, default value} map
static const struct {
  enum settings_type type;
  const char *key_str;
  const union {
    const char *str;
    const int32_t i32;
  } default_value;
} settings_table[] = {
  [kSettingsWifiSsid] =
    {
      kSettingsTypeString,
      "wifi_ssid",
      {
        .str = "",
      },
    },
  [kSettingsWifiPassword] =
    {
      kSettingsTypeString,
      "wifi_password",
      {
        .str = "",
      },
    },
  [kSettingsProjectKey] =
    {
      kSettingsTypeString,
      "project_key",
      {
        .str = CONFIG_MEMFAULT_PROJECT_KEY,
      },
    },
  [kSettingsLedBrightness] =
    {
      kSettingsTypeI32,
      "led_brightness",
      {
        .i32 = 5,
      },
    },
  [kSettingsLedBlinkIntervalMs] =
    {
      kSettingsTypeI32,
      "led_blink_ms",
      {
        .i32 = 500,
      },
    },
};

static bool prv_settings_key_is_valid(enum settings_key key) {
  return key < (sizeof(settings_table) / sizeof(settings_table[0]));
}

static esp_err_t prv_open_nvs(nvs_handle_t *nvs_handle) {
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
  err = nvs_open("storage", NVS_READWRITE, nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

#if __GNUC__ >= 11
__attribute__((access(write_only, 2)))
#endif
int settings_get(enum settings_key key, void *value,
                                                        size_t *len) {
  if (!prv_settings_key_is_valid(key)) {
    ESP_LOGE(__func__, "Invalid key: %d", key);
    return ESP_ERR_INVALID_ARG;
  }

  // Initialize NVS
  nvs_handle_t nvs_handle;
  esp_err_t err = prv_open_nvs(&nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  ESP_LOGD(__func__, "Opened NVS handle");

  // Read
  switch (settings_table[key].type) {
    case kSettingsTypeString:
      err = nvs_get_str(nvs_handle, settings_table[key].key_str, (char *)value, len);
      break;
    case kSettingsTypeI32:
      err = nvs_get_i32(nvs_handle, settings_table[key].key_str, (int32_t *)value);
      break;
    default:
      ESP_LOGE(__func__, "Invalid type: %d", settings_table[key].type);
      err = ESP_ERR_INVALID_ARG;
      goto close;
      break;
  }
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGD(__func__, "%d not in store, loading default", key);
    err = ESP_OK;
    switch (settings_table[key].type) {
      case kSettingsTypeString:
        strncpy(value, settings_table[key].default_value.str, *len - 1);
        *len = strlen(settings_table[key].default_value.str);
        break;
      case kSettingsTypeI32:
        *(int32_t *)value = settings_table[key].default_value.i32;
        break;
      default:
        ESP_LOGE(__func__, "Invalid type: %d", settings_table[key].type);
        err = ESP_ERR_INVALID_ARG;
        goto close;
        break;
    }

  } else if (err != ESP_OK) {
    ESP_LOGE(__func__, "failed reading %d", key);
  }

// Close
close:
  nvs_close(nvs_handle);

  return err;
}

#if __GNUC__ >= 11
__attribute__((access(read_only, 2, 3)))
#endif
int settings_set(enum settings_key key, const void *value, size_t len) {
  if (!prv_settings_key_is_valid(key)) {
    ESP_LOGE(__func__, "Invalid key: %d", key);
    return ESP_ERR_INVALID_ARG;
  }

  // Initialize NVS
  nvs_handle_t nvs_handle;
  esp_err_t err = prv_open_nvs(&nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  ESP_LOGD(__func__, "Opened NVS handle");

  // Write
  switch (settings_table[key].type) {
    case kSettingsTypeString:
      err = nvs_set_str(nvs_handle, settings_table[key].key_str, value);
      break;
    case kSettingsTypeI32:
      err = nvs_set_i32(nvs_handle, settings_table[key].key_str, *(int32_t *)value);
      break;
    default:
      ESP_LOGE(__func__, "Invalid type: %d", settings_table[key].type);
      err = ESP_ERR_INVALID_ARG;
      goto close;
      break;
  }
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) writing key %d to NVS!", esp_err_to_name(err), key);
  } else {
    ESP_LOGD(__func__, "Wrote key %d to NVS", key);
  }

  // Commit written value.
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) committing NVS!", esp_err_to_name(err));
  } else {
    ESP_LOGD(__func__, "Successfully wrote key %d to NVS", key);
  }

// Close
close:
  nvs_close(nvs_handle);

  return err;
}

static struct {
  struct arg_str *key;
  struct arg_end *end;
} s_get_args;

static struct {
  struct arg_str *key;
  struct arg_str *value;
  struct arg_end *end;
} s_set_args;

static int prv_string_to_key(const char *str, enum settings_key *key) {
  for (size_t i = 0; i < sizeof(settings_table) / sizeof(settings_table[0]); i++) {
    if (strcmp(str, settings_table[i].key_str) == 0) {
      *key = i;
      return 0;
    }
  }

  return 1;
}

static int prv_get_and_print_setting(enum settings_key key) {
  // settings longer than 100 not supported
  size_t len = 100;

  char *value = malloc(len);
  if (value == NULL) {
    ESP_LOGE(__func__, "Error allocating %d bytes", len);
    return 1;
  }

  int err = settings_get(key, value, &len);
  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) getting key %d", esp_err_to_name(err), key);
    return 1;
  }

  switch (settings_table[key].type) {
    case kSettingsTypeString:
      printf("%s: %s\n", settings_table[key].key_str, value);
      break;
    case kSettingsTypeI32:
      printf("%s: %" PRIi32 "\n", settings_table[key].key_str, *(int32_t *)value);
      break;
    default:
      break;
  }

  free(value);

  return 0;
}

static int prv_get_cmd(int argc, char **argv) {
  if (argc == 1) {
    // print every setting in the table
    for (size_t i = 0; i < sizeof(settings_table) / sizeof(settings_table[0]); i++) {
      (void)prv_get_and_print_setting(i);
    }
    return 0;
  }

  int nerrors = arg_parse(argc, argv, (void **)&s_get_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, s_get_args.end, argv[0]);
    return 1;
  }

  enum settings_key key;
  if (prv_string_to_key(s_get_args.key->sval[0], &key)) {
    ESP_LOGE(__func__, "Invalid key: %s", s_get_args.key->sval[0]);
    return 1;
  }

  return prv_get_and_print_setting(key);
}

static int prv_set_cmd(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&s_set_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, s_set_args.end, argv[0]);
    return 1;
  }

  enum settings_key key;
  if (prv_string_to_key(s_set_args.key->sval[0], &key)) {
    ESP_LOGE(__func__, "Invalid key: %s", s_set_args.key->sval[0]);
    return 1;
  }

  int err = ESP_OK;
  switch (settings_table[key].type) {
    case kSettingsTypeString:
      err = settings_set(key, s_set_args.value->sval[0], strlen(s_set_args.value->sval[0]));
      break;
    case kSettingsTypeI32:
      // convert string to int
      {
        int32_t i32;
        if (sscanf(s_set_args.value->sval[0], "%" SCNi32, &i32) != 1) {
          ESP_LOGE(__func__, "Invalid value: %s", s_set_args.value->sval[0]);
          return 1;
        }
        err = settings_set(key, &i32, sizeof(i32));
      }
      break;
    default:
      return 1;
      break;
  }

  if (err != ESP_OK) {
    ESP_LOGE(__func__, "Error (%s) setting key %d", esp_err_to_name(err), key);
    return 1;
  }

  return 0;
}

void settings_register_shell_commands(void) {
  s_get_args.key = arg_str1(NULL, NULL, "<key>", "Key name to get");
  s_get_args.end = arg_end(1);
  static const esp_console_cmd_t s_get_cmd = {
    .command = "settings_get",
    .help = "Get a setting",
    .hint = NULL,
    .func = prv_get_cmd,
    .argtable = &s_get_args,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&s_get_cmd));

  s_set_args.key = arg_str1(NULL, NULL, "<key>", "Key name to set");
  s_set_args.value = arg_str1(NULL, NULL, "<value>", "Value to set (string or i32)");
  s_set_args.end = arg_end(2);
  static const esp_console_cmd_t s_set_cmd = {
    .command = "settings_set",
    .help = "Set a setting",
    .hint = NULL,
    .func = prv_set_cmd,
    .argtable = &s_set_args,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&s_set_cmd));
}
