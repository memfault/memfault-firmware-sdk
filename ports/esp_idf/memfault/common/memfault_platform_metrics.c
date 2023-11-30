//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port of dependency functions for Memfault metrics subsystem using FreeRTOS.
//!
//! @note For test purposes, the heartbeat interval can be changed to a faster period
//! by using the following CFLAG:
//!   -DMEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS=15

#include <string.h>

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "memfault/core/math.h"
#include "memfault/core/reboot_tracking.h"
#include "memfault/esp_port/metrics.h"
#include "memfault/esp_port/version.h"
#include "memfault/metrics/metrics.h"
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

#if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)

  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
int32_t s_min_rssi;
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)

  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    // This is a practical maximum RSSI value. In reality, the RSSI value is
    // will likely be much lower. A value in the mid -60s is considered very good
    // for example. An RSSI of -20 would be a device essentially on top of the AP.
    #define MAXIMUM_RSSI -10
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
#endif    // CONFIG_MEMFAULT_ESP_WIFI_METRICS

MEMFAULT_WEAK
void memfault_esp_metric_timer_dispatch(MemfaultPlatformTimerCallback handler) {
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
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  wifi_event_bss_rssi_low_t *rssi_data;
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)

  switch (event_id) {
    case WIFI_EVENT_STA_CONNECTED:
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
      // The RSSI threshold needs to be set when WIFI is already initialized.
      // This event is the most reliable way to know that we're already in that
      // state.
      ESP_ERROR_CHECK(esp_wifi_set_rssi_threshold(MAXIMUM_RSSI));
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
      MEMFAULT_METRIC_TIMER_START(wifi_connected_time_ms);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      MEMFAULT_METRIC_ADD(wifi_disconnect_count, 1);
      MEMFAULT_METRIC_TIMER_STOP(wifi_connected_time_ms);

      break;
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    case WIFI_EVENT_STA_BSS_RSSI_LOW:
      rssi_data = (wifi_event_bss_rssi_low_t *)event_data;

      s_min_rssi = MEMFAULT_MIN(rssi_data->rssi, s_min_rssi);
      esp_wifi_set_rssi_threshold(s_min_rssi);
      break;
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    default:
      break;
  }
}

static void prv_register_event_handler(void) {
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

  ESP_ERROR_CHECK(
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &metric_event_handler, NULL));
}

static void prv_collect_oui(void) {
  wifi_ap_record_t ap_info;
  static uint8_t s_memfault_ap_bssid[sizeof(ap_info.bssid)];
  esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
  if (err == ESP_OK) {
    // only set the metric if the AP MAC changed
    if (memcmp(s_memfault_ap_bssid, ap_info.bssid, sizeof(s_memfault_ap_bssid)) != 0) {
      char oui[9];
      snprintf(oui, sizeof(oui), "%02x:%02x:%02x", ap_info.bssid[0], ap_info.bssid[1],
               ap_info.bssid[2]);
      MEMFAULT_METRIC_SET_STRING(wifi_ap_oui, oui);
      memcpy(s_memfault_ap_bssid, ap_info.bssid, sizeof(s_memfault_ap_bssid));
    }
  }
}

static void prv_collect_wifi_metrics(void) {
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
  if (s_min_rssi <= MAXIMUM_RSSI) {
    MEMFAULT_METRIC_SET_SIGNED(wifi_sta_min_rssi, s_min_rssi);
    // Error checking is ignored here, as it's possible that WIFI is not
    // intialized yet at time of heartbeat. In that case, the RSSI threshold
    // will be set reset on the first connection, and any errors here can be
    // safely ignored.
    (void)esp_wifi_set_rssi_threshold(MAXIMUM_RSSI);
    s_min_rssi = 0;
  }
  #endif  // ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)

  // Collect AP OUI
  prv_collect_oui();
}
#endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS

#if defined(CONFIG_MEMFAULT_ESP_HEAP_METRICS)

static void prv_record_heap_metrics(void) {
  multi_heap_info_t heap_info = {0};
  heap_caps_get_info(&heap_info, MALLOC_CAP_DEFAULT);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_free_bytes, heap_info.total_free_bytes);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_largest_free_block_bytes, heap_info.largest_free_block);
  MEMFAULT_METRIC_SET_UNSIGNED(heap_allocated_blocks_count, heap_info.allocated_blocks);
  // lifetime minimum free bytes. see caveat here:
  // https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32/api-reference/system/mem_alloc.html#_CPPv431heap_caps_get_minimum_free_size8uint32_t
  MEMFAULT_METRIC_SET_UNSIGNED(heap_min_free_bytes, heap_info.minimum_free_bytes);
}

#endif  // CONFIG_MEMFAULT_ESP_HEAP_METRICS

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

#if defined(CONFIG_MEMFAULT_ESP_WIFI_METRICS)
  prv_register_event_handler();
#endif  // CONFIG_MEMFAULT_ESP_WIFI_METRICS

  return true;
}

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

#if defined(CONFIG_MEMFAULT_ESP_HEAP_METRICS)
  prv_record_heap_metrics();
#endif  // CONFIG_MEMFAULT_ESP_HEAP_METRICS
}
