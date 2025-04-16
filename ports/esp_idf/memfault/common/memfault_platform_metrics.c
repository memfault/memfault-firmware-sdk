//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! A port of dependency functions for Memfault metrics subsystem using FreeRTOS.
//!
//! @note For test purposes, the heartbeat interval can be changed to a faster period
//! by using the following CFLAG:
//!   -DMEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS=15

#include <inttypes.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "memfault/core/debug_log.h"
#include "memfault/core/math.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/esp_port/metrics.h"
#include "memfault/metrics/connectivity.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/connectivity.h"
#include "memfault/metrics/platform/timer.h"
#include "sdkconfig.h"

#if defined(CONFIG_MEMFAULT_LWIP_METRICS)
  #include "memfault/ports/lwip/metrics.h"
#endif  // CONFIG_MEMFAULT_LWIP_METRICS

#if defined(CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS)
  #include "memfault/ports/freertos/metrics.h"
#endif  // CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS

#if defined(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  #include "memfault/ports/mbedtls/metrics.h"
#endif  // CONFIG_MEMFAULT_MBEDTLS_METRICS

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
  #include "driver/temperature_sensor.h"
  #include "soc/clk_tree_defs.h"
#endif  // CONFIG_MEMFAULT_METRICS_CPU_TEMP

#if defined(CONFIG_MEMFAULT_METRICS_CHIP_ENABLE)
  #include "esp_flash.h"
  #include "hal/efuse_hal.h"
#endif

#if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)

int32_t s_min_rssi;
  // This is a practical maximum RSSI value. In reality, the RSSI value is
  // will likely be much lower. A value in the mid -60s is considered very good
  // for example. An RSSI of -20 would be a device essentially on top of the AP.
  #define MAXIMUM_RSSI -10
#endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS

MEMFAULT_WEAK void memfault_esp_metric_timer_dispatch(MemfaultPlatformTimerCallback handler) {
  if (handler == NULL) {
    return;
  }
  handler();
}

static void prv_metric_timer_handler(void *arg) {
  memfault_reboot_tracking_reset_crash_count();

  // NOTE: This timer runs once per MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS where the default is
  // once per hour.
  MemfaultPlatformTimerCallback *metric_timer_handler = (MemfaultPlatformTimerCallback *)arg;
  memfault_esp_metric_timer_dispatch(metric_timer_handler);
}

#if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)
static void metric_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                 void *event_data) {
  wifi_event_bss_rssi_low_t *rssi_data;

  switch (event_id) {
    case WIFI_EVENT_STA_START:
  #if defined(CONFIG_MEMFAULT_ESP_WIFI_CONNECTIVITY_TIME_METRICS)
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Started);
  #endif
      break;

    case WIFI_EVENT_STA_CONNECTED:
      // The RSSI threshold needs to be set when WIFI is already initialized.
      // This event is the most reliable way to know that we're already in that
      // state.
      ESP_ERROR_CHECK(esp_wifi_set_rssi_threshold(MAXIMUM_RSSI));
      MEMFAULT_METRIC_TIMER_START(wifi_connected_time_ms);

  #if defined(CONFIG_MEMFAULT_ESP_WIFI_CONNECTIVITY_TIME_METRICS)
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Connected);
  #endif
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      MEMFAULT_METRIC_ADD(wifi_disconnect_count, 1);
      MEMFAULT_METRIC_TIMER_STOP(wifi_connected_time_ms);

  #if defined(CONFIG_MEMFAULT_ESP_WIFI_CONNECTIVITY_TIME_METRICS)
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_ConnectionLost);
  #endif
      break;

    case WIFI_EVENT_STA_BSS_RSSI_LOW:
      rssi_data = (wifi_event_bss_rssi_low_t *)event_data;

      s_min_rssi = MEMFAULT_MIN(rssi_data->rssi, s_min_rssi);
      esp_wifi_set_rssi_threshold(s_min_rssi);
      break;

    default:
      break;
  }
}

static void prv_register_event_handler(void) {
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

  ESP_ERROR_CHECK(
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &metric_event_handler, NULL));
}

static void prv_collect_oui(wifi_ap_record_t *ap_info) {
  static uint8_t s_memfault_ap_bssid[sizeof(ap_info->bssid)];
  // only set the metric if the AP MAC changed
  if (memcmp(s_memfault_ap_bssid, ap_info->bssid, sizeof(s_memfault_ap_bssid)) != 0) {
    char oui[9];
    snprintf(oui, sizeof(oui), "%02x:%02x:%02x", ap_info->bssid[0], ap_info->bssid[1],
             ap_info->bssid[2]);
    MEMFAULT_METRIC_SET_STRING(wifi_ap_oui, oui);
    memcpy(s_memfault_ap_bssid, ap_info->bssid, sizeof(s_memfault_ap_bssid));
  }
}

// Maximum allowable string length is limited by the metric using this,
static const char *auth_mode_strings[] = {
  // clang-format off
  [WIFI_AUTH_OPEN] = "OPEN",
  [WIFI_AUTH_WEP] = "WEP",
  [WIFI_AUTH_WPA_PSK] = "WPA-PSK",
  [WIFI_AUTH_WPA2_PSK] = "WPA2-PSK",
  [WIFI_AUTH_WPA_WPA2_PSK] = "WPA-WPA2-PSK",
  [WIFI_AUTH_WPA2_ENTERPRISE] = "WPA2-ENTERPRISE",
  [WIFI_AUTH_WPA3_PSK] = "WPA3-PSK",
  [WIFI_AUTH_WPA2_WPA3_PSK] = "WPA2-WPA3-PSK",
  [WIFI_AUTH_WAPI_PSK] = "WAPI-PSK",
  [WIFI_AUTH_WPA3_ENT_192] = "WPA3-ENT-192",
  // other auth modes are not supported, and will show "UNKNOWN"
  // clang-format on
};

static const char *prv_esp_idf_authomode_to_str(wifi_auth_mode_t auth_mode) {
  if (auth_mode >= MEMFAULT_ARRAY_SIZE(auth_mode_strings)) {
    return "UNKNOWN";
  }

  const char *authmode = auth_mode_strings[auth_mode];

  // The auth mode enum is not contiguous, due to different entries supported on
  // different ESP-IDF versions, so we need to check for NULL. We care most
  // about the most popular auth modes, so we don't need to be exhaustive.
  if (authmode == NULL) {
    return "UNKNOWN";
  }

  return authmode;
}

static void prv_collect_wifi_version(wifi_ap_record_t *ap_info) {
  // Map the phy_ bits to a Wi-Fi Version integer
  const char *wifi_version = "unknown";

  if (ap_info->phy_11n) {
    wifi_version = "802.11n";
  // phy_11ac and phy_11a were added in ESP-IDF 5.3
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
  } else if (ap_info->phy_11ac) {
    wifi_version = "802.11ac";
  } else if (ap_info->phy_11a) {
    wifi_version = "802.11a";
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
  // phy_11ax was added in ESP-IDF 5.1
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
  } else if (ap_info->phy_11ax) {
    wifi_version = "802.11ax";
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
  } else if (ap_info->phy_11g) {
    wifi_version = "802.11g";
  } else if (ap_info->phy_11b) {
    wifi_version = "802.11b";
  }

  MEMFAULT_METRIC_SET_STRING(wifi_standard_version, wifi_version);
}

static void prv_collect_wifi_metrics(void) {
  if (s_min_rssi <= MAXIMUM_RSSI) {
    MEMFAULT_METRIC_SET_SIGNED(wifi_sta_min_rssi, s_min_rssi);
    // Error checking is ignored here, as it's possible that WIFI is not
    // initialized yet at time of heartbeat. In that case, the RSSI threshold
    // will be set reset on the first connection, and any errors here can be
    // safely ignored.
    (void)esp_wifi_set_rssi_threshold(MAXIMUM_RSSI);
    s_min_rssi = 0;
  }

  wifi_ap_record_t ap_info;
  esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
  if (err != ESP_OK) {
    // Other metrics require this data, exit early if we can't get it
    return;
  }

  // Primary channel
  MEMFAULT_METRIC_SET_UNSIGNED(wifi_primary_channel, ap_info.primary);

  const char *auth_mode_str = prv_esp_idf_authomode_to_str(ap_info.authmode);
  MEMFAULT_METRIC_SET_STRING(wifi_auth_mode, auth_mode_str);

  prv_collect_wifi_version(&ap_info);

  prv_collect_oui(&ap_info);
}
#endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
static void prv_collect_memory_usage_metrics(void) {
  multi_heap_info_t heap_info = { 0 };
  heap_caps_get_info(&heap_info, MALLOC_CAP_DEFAULT);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_free_bytes, heap_info.total_free_bytes);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_largest_free_block_bytes, heap_info.largest_free_block);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_allocated_blocks_count, heap_info.allocated_blocks);
  // Metrics below use lifetime minimum free bytes. see caveat here:
  // https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32/api-reference/system/mem_alloc.html#_CPPv431heap_caps_get_minimum_free_size8uint32_t
  MEMFAULT_METRIC_SET_UNSIGNED(heap_min_free_bytes, heap_info.minimum_free_bytes);
  uint32_t total_heap_size = heap_info.total_free_bytes + heap_info.total_allocated_bytes;

  // Range is 0-10000 for 0.00-100.00%
  // Multiply by 100 to get us 2 decimals of precision via the scale factor and then by 100 again
  // for percentage conversion.
  MEMFAULT_METRIC_SET_UNSIGNED(
    memory_pct_max,
    ((uint32_t)((total_heap_size - heap_info.minimum_free_bytes) * 10000.0f) / total_heap_size));
}
#endif  // CONFIG_MEMFAULT_METRICS_MEMORY_USAGE

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &prv_metric_timer_handler,
    .arg = callback,
    .name = "mflt",
  };

#if defined(CONFIG_MEMFAULT_AUTOMATIC_INIT)
  // This is only needed if CONFIG_MEMFAULT_AUTOMATIC_INIT is enabled. Normally
  // esp_timer_init() is called during the system init sequence, before
  // app_main(), but if memfault_boot() is running in the system init stage, it
  // can potentially run before esp_timer_init() has been called.
  //
  // Ignore return value; this function should be safe to call multiple times
  // during system init, but needs to called before we create any timers.
  // See implementation here (may change by esp-idf version!):
  // https://github.com/espressif/esp-idf/blob/master/components/esp_timer/src/esp_timer.c#L431-L460
  (void)esp_timer_init();
#endif

  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

  const int64_t us_per_sec = 1000000;
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, period_sec * us_per_sec));

  return true;
}

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
static void prv_collect_temperature_metric(void) {
  // See documentation here:
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/temp_sensor.html
  static temperature_sensor_handle_t temp_handle = NULL;
  temperature_sensor_config_t temp_sensor = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
  esp_err_t err;

  do {
    if (!temp_handle) {
      err = temperature_sensor_install(&temp_sensor, &temp_handle);
      if (err != ESP_OK) {
        MEMFAULT_LOG_ERROR("Failed to install temperature sensor: %d", err);
        break;
      }
    }
    // Enable temperature sensor
    err = temperature_sensor_enable(temp_handle);
    if (err != ESP_OK) {
      MEMFAULT_LOG_ERROR("Failed to enable temperature sensor: %d", err);
      break;
    }
    // Get converted sensor data
    float tsens_out;
    err = temperature_sensor_get_celsius(temp_handle, &tsens_out);
    if (err != ESP_OK) {
      MEMFAULT_LOG_ERROR("Failed to get temperature sensor data: %d", err);
      break;
    } else {
      MEMFAULT_METRIC_SET_SIGNED(thermal_cpu_c, (int32_t)(tsens_out * 10.0f));
    }
  } while (0);

  // Disable the temperature sensor if it is not needed and save the power
  (void)temperature_sensor_disable(temp_handle);
}
#endif  // CONFIG_MEMFAULT_METRICS_CPU_TEMP

#if defined(CONFIG_MEMFAULT_METRICS_CHIP_ENABLE)
static void prv_collect_chip_metrics(void) {
  static bool did_collect = false;

  if (did_collect) {
    return;
  }

  uint32_t flash_chip_id;
  esp_err_t err = esp_flash_read_id(NULL, &flash_chip_id);
  if (err == ESP_OK) {
    // flash_chip_id is 24 bits. convert to hex.
    char flash_chip_id_str[7];
    snprintf(flash_chip_id_str, sizeof(flash_chip_id_str), "%06" PRIx32, flash_chip_id);
    // Set the chip id metric
    MEMFAULT_METRIC_SET_STRING(spi_flash_chip_id, flash_chip_id_str);
  }

  uint32_t efuse_chip_id = efuse_hal_chip_revision();
  // Chip version in format: Major * 100 + Minor
  // e.g. Major = 3, Minor = 0 -> 300
  // Convert to string with leading CONFIG_IDF_TARGET, eg "esp32s3-3.3"
  char chip_id_str[sizeof(CONFIG_IDF_TARGET) + sizeof("-00.00")];
  const uint8_t major = efuse_chip_id / 100;
  const uint8_t minor = efuse_chip_id % 100;
  snprintf(chip_id_str, sizeof(chip_id_str), CONFIG_IDF_TARGET "-%u.%u", major, minor);

  MEMFAULT_METRIC_SET_STRING(esp_chip_revision, chip_id_str);

  // We only need to collect this one time on boot
  did_collect = true;
}
#endif  // CONFIG_MEMFAULT_METRICS_CHIP_ENABLE

void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if defined(CONFIG_MEMFAULT_LWIP_METRICS)
  memfault_lwip_heartbeat_collect_data();
#endif  // CONFIG_MEMFAULT_LWIP_METRICS

#if defined(CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS)
  MEMFAULT_STATIC_ASSERT(
    MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS <= (60 * 60),
    "Heartbeat must be an hour or less for runtime metrics to mitigate counter overflow");

  memfault_freertos_port_task_runtime_metrics();
#endif  // CONFIG_MEMFAULT_FREERTOS_TASK_RUNTIME_STATS

#if defined(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  memfault_mbedtls_heartbeat_collect_data();
#endif  // CONFIG_MEMFAULT_MBEDTLS_METRICS

#if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)
  prv_collect_wifi_metrics();
#endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS

#if defined(CONFIG_MEMFAULT_METRICS_MEMORY_USAGE)
  prv_collect_memory_usage_metrics();
#endif  // CONFIG_MEMFAULT_METRICS_MEMORY_USAGE

#if defined(CONFIG_MEMFAULT_METRICS_CPU_TEMP)
  prv_collect_temperature_metric();
#endif  // CONFIG_MEMFAULT_METRICS_CPU_TEMP

#if defined(CONFIG_MEMFAULT_METRICS_CHIP_ENABLE)
  prv_collect_chip_metrics();
#endif  // CONFIG_MEMFAULT_METRICS_CHIP_ENABLE
}

#if defined(CONFIG_MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT)
//! Register handlers for WiFi events
void memfault_platform_metrics_connectivity_boot(void) {
  #if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)
  prv_register_event_handler();
  #endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS
}
#endif  // CONFIG_MEMFAULT_PLATFORM_METRICS_CONNECTIVITY_BOOT

#if defined(CONFIG_MEMFAULT_WRAP_EVENT_LOOP_CREATE_DEFAULT)
esp_err_t __real_esp_event_loop_create_default(void);

esp_err_t __wrap_esp_event_loop_create_default(void) {
  esp_err_t result = __real_esp_event_loop_create_default();
  return result == ESP_ERR_INVALID_STATE ? ESP_OK : result;
}

// Ensure the substituted function signature matches the original function
_Static_assert(__builtin_types_compatible_p(__typeof__(&esp_event_loop_create_default),
                                            __typeof__(&__wrap_esp_event_loop_create_default)) &&
                 __builtin_types_compatible_p(__typeof__(&esp_event_loop_create_default),
                                              __typeof__(&__real_esp_event_loop_create_default)),
               "Error: check esp_event_loop_create_default function signature");
#endif  // defined(CONFIG_MEMFAULT_WRAP_EVENT_LOOP_CREATE_DEFAULT)
