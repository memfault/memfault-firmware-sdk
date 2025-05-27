//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Implementation for Memfault OTA Session Metrics

#include "ota_session_metrics.h"

#include <stdint.h>

#include "app_memfault_transport.h"
#include "esp_idf_version.h"
#include "esp_wifi.h"
#include "lwip/stats.h"
#include "memfault/components.h"
#include "memfault/esp_port/metrics.h"
#include "memfault/ports/mbedtls/metrics.h"
#include "sdkconfig.h"

static struct ota_session_metrics_lwip_stats_prev {
  uint32_t tx;
  uint32_t rx;
} s_ota_session_metrics_lwip_stats_prev;

static struct ota_session_metrics_mbedtls_stats {
  uint32_t sent;
  uint32_t recv;
} s_ota_session_metrics_mbedtls_stats;

#if defined(CONFIG_MEMFAULT_METRICS_FLASH_ENABLE)
static sMfltFlashCounters s_ota_session_metrics_flash_counters = { 0 };
#endif

void ota_session_metrics_start(void) {
  // save starting tcp_tx_count and tcp_rx_count metrics
  s_ota_session_metrics_lwip_stats_prev = (struct ota_session_metrics_lwip_stats_prev){
    .tx = lwip_stats.tcp.xmit,
    .rx = lwip_stats.tcp.recv,
  };

  // reset mbedtls tx/rx stats
  s_ota_session_metrics_mbedtls_stats = (struct ota_session_metrics_mbedtls_stats){ 0 };

#if defined(CONFIG_MEMFAULT_METRICS_FLASH_ENABLE)
  // initialize baseline for flash counters
  (void)memfault_platform_metrics_get_flash_counters(&s_ota_session_metrics_flash_counters);
#endif

  (void)MEMFAULT_METRICS_SESSION_START(ota);
}

extern int __real_mbedtls_net_send(void *ctx, unsigned char const *buf, size_t len);
int __wrap_mbedtls_net_send(void *ctx, unsigned char const *buf, size_t len) {
  int rv = __real_mbedtls_net_send(ctx, buf, len);
  if (rv > 0) {
    s_ota_session_metrics_mbedtls_stats.sent += rv;
  }
  return rv;
}

extern int __real_mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len);
int __wrap_mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
  int rv = __real_mbedtls_net_recv(ctx, buf, len);
  if (rv > 0) {
    s_ota_session_metrics_mbedtls_stats.recv += rv;
  }
  return rv;
}

void ota_session_metrics_end(int ota_error_code) {
  // error code
  MEMFAULT_METRIC_SESSION_SET_SIGNED(ota_error_code, ota, ota_error_code);

  // Read last recorded station RSSI. this is reset every heartbeat in
  // ports/esp_idf/memfault/common/memfault_platform_metrics.c, so this gives us
  // a reasonably recent RSSI reading. Note: esp_wifi_sta_get_rssi() was added
  // in v5.2 and back-ported to 4.4.8/5.0.7/5.1.5, so there's no version check
  // here.
  int rssi;
  esp_err_t err = esp_wifi_sta_get_rssi(&rssi);
  if (err == ESP_OK) {
    MEMFAULT_METRIC_SESSION_SET_SIGNED(ota_wifi_rssi, ota, rssi);
  }

  // lwip TCP tx + rx packet counts
  uint32_t tx_count = lwip_stats.tcp.xmit - s_ota_session_metrics_lwip_stats_prev.tx;
  uint32_t rx_count = lwip_stats.tcp.recv - s_ota_session_metrics_lwip_stats_prev.rx;
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ota_tcp_tx_count, ota, tx_count);
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ota_tcp_rx_count, ota, rx_count);

  // mbedtls net sent + received byte counts
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ota_tls_sent_bytes, ota,
                                       s_ota_session_metrics_mbedtls_stats.sent);
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ota_tls_recv_bytes, ota,
                                       s_ota_session_metrics_mbedtls_stats.recv);

#if defined(CONFIG_MEMFAULT_MBEDTLS_METRICS)
  // mbedtls max heap usage
  sMemfaultMbedtlsMetricData mbedtls_stats;
  memfault_mbedtls_heartbeat_get_data(&mbedtls_stats);
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(ota_mbedtls_mem_max_bytes, ota, mbedtls_stats.mem_max_bytes);
#endif  // CONFIG_MEMFAULT_MBEDTLS_METRICS

#if defined(CONFIG_MEMFAULT_METRICS_FLASH_ENABLE)
  sMfltFlashCounters flash_counters =
    memfault_platform_metrics_get_flash_counters(&s_ota_session_metrics_flash_counters);
  MEMFAULT_METRIC_SESSION_ADD(flash_spi_erase_bytes, ota, flash_counters.erase_bytes);
  MEMFAULT_METRIC_SESSION_ADD(flash_spi_write_bytes, ota, flash_counters.write_bytes);
#endif  // CONFIG_MEMFAULT_METRICS_FLASH_ENABLE

  (void)MEMFAULT_METRICS_SESSION_END(ota);

  // after session end, issue a chunk post to upload the data to memfault
  (void)app_memfault_transport_send_chunks();
}
