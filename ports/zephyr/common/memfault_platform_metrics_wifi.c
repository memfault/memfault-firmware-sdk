//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)

#include <stdio.h>
#include MEMFAULT_ZEPHYR_INCLUDE(net/net_mgmt.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/wifi.h)
#include MEMFAULT_ZEPHYR_INCLUDE(net/wifi_mgmt.h)

#include <stdbool.h>

#include "memfault/core/debug_log.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/zephyr/version.h"
// clang-format on

const char *prv_link_mode_to_str(enum wifi_link_mode link_mode) {
  switch (link_mode) {
    case WIFI_0:
      return "802.11";
    case WIFI_1:
      return "802.11b";
    case WIFI_2:
      return "802.11a";
    case WIFI_3:
      return "802.11g";
    case WIFI_4:
      return "802.11n";
    case WIFI_5:
      return "802.11ac";
    case WIFI_6:
      return "802.11ax";
    case WIFI_6E:
      return "802.11ax/6GHz";
    case WIFI_7:
      return "802.11be";
    case WIFI_LINK_MODE_UNKNOWN:
    default:
      return "unknown";
  }
}

static const char *prv_wifi_security_type_to_string(enum wifi_security_type type) {
  switch (type) {
    case WIFI_SECURITY_TYPE_NONE:
      return "NONE";
    case WIFI_SECURITY_TYPE_PSK:
      return "WPA2-PSK";
    case WIFI_SECURITY_TYPE_PSK_SHA256:
      return "WPA2-PSK-SHA256";
    case WIFI_SECURITY_TYPE_SAE:
      // case WIFI_SECURITY_TYPE_SAE_HNP:
      return "WPA3-SAE";
    case WIFI_SECURITY_TYPE_SAE_H2E:
      return "WPA3-SAE-H2E";
    case WIFI_SECURITY_TYPE_SAE_AUTO:
      return "WPA3-SAE-AUTO";
    case WIFI_SECURITY_TYPE_WAPI:
      return "WAPI";
    case WIFI_SECURITY_TYPE_EAP:
      // case WIFI_SECURITY_TYPE_EAP_TLS:
      return "EAP";
    case WIFI_SECURITY_TYPE_WEP:
      return "WEP";
    case WIFI_SECURITY_TYPE_WPA_PSK:
      return "WPA-PSK";
    case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
      return "WPA-AUTO-PERSONAL";
    case WIFI_SECURITY_TYPE_DPP:
      return "DPP";
    case WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2:
      return "EAP-PEAP-MSCHAPV2";
    case WIFI_SECURITY_TYPE_EAP_PEAP_GTC:
      return "EAP-PEAP-GTC";
    case WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2:
      return "EAP-TTLS-MSCHAPV2";
    case WIFI_SECURITY_TYPE_EAP_PEAP_TLS:
      return "EAP-PEAP-TLS";
// only for zephyr >= 4.1.0
#if MEMFAULT_ZEPHYR_VERSION_GTE_STRICT(4, 1)
    case WIFI_SECURITY_TYPE_FT_PSK:
      return "FT-PSK";
    case WIFI_SECURITY_TYPE_FT_SAE:
      return "FT-SAE";
    case WIFI_SECURITY_TYPE_FT_EAP:
      return "FT-EAP";
    case WIFI_SECURITY_TYPE_FT_EAP_SHA384:
      return "FT-EAP-SHA384";
    case WIFI_SECURITY_TYPE_SAE_EXT_KEY:
      return "SAE-EXT-KEY";
#endif
    default:
      return "UNKNOWN";
  }
}

const char *prv_wifi_frequency_band_to_str(enum wifi_frequency_bands band) {
  switch (band) {
    case WIFI_FREQ_BAND_2_4_GHZ:
      return "2.4";
    case WIFI_FREQ_BAND_5_GHZ:
      return "5";
    case WIFI_FREQ_BAND_6_GHZ:
      return "6";
    default:
      return "x";
  }
}

static void prv_record_wifi_connection_metrics(struct net_if *iface) {
  struct wifi_iface_status status = { 0 };

  if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status))) {
    return;
  }

  MEMFAULT_METRIC_SET_STRING(wifi_standard_version, prv_link_mode_to_str(status.link_mode));
  MEMFAULT_METRIC_SET_STRING(wifi_security_type, prv_wifi_security_type_to_string(status.security));
  MEMFAULT_METRIC_SET_STRING(wifi_frequency_band, prv_wifi_frequency_band_to_str(status.band));
  MEMFAULT_METRIC_SET_UNSIGNED(wifi_primary_channel, status.channel);

  if (status.iface_mode == WIFI_MODE_INFRA) {
    MEMFAULT_METRIC_SET_SIGNED(wifi_sta_rssi, status.rssi);
  }

  MEMFAULT_METRIC_SET_UNSIGNED(wifi_beacon_interval, status.beacon_interval);
  MEMFAULT_METRIC_SET_UNSIGNED(wifi_dtim_interval, status.dtim_period);
  MEMFAULT_METRIC_SET_UNSIGNED(wifi_twt_capable, status.twt_capable);

#if MEMFAULT_ZEPHYR_VERSION_GTE_STRICT(4, 1)
  // some devices will not have this value set
  if (status.current_phy_tx_rate != 0) {
    MEMFAULT_METRIC_SET_UNSIGNED(wifi_tx_rate_mbps, status.current_phy_tx_rate);
  }
#endif

  char oui[9];
  snprintf(oui, sizeof(oui), "%02x:%02x:%02x", status.bssid[0], status.bssid[1], status.bssid[2]);
  MEMFAULT_METRIC_SET_STRING(wifi_ap_oui, oui);
}

static void prv_wifi_event_callback(struct net_mgmt_event_callback *cb,
#if MEMFAULT_ZEPHYR_VERSION_GT(4, 1)
                                    uint64_t mgmt_event,
#else
                                    uint32_t mgmt_event,
#endif
                                    struct net_if *iface) {
  switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: {
      const struct wifi_status *status = (const struct wifi_status *)cb->info;
      if (status->status) {
        // Future: Record connect errors
      } else {
        // start connected-time timer
        MEMFAULT_METRIC_TIMER_START(wifi_connected_time_ms);

        // record connection metrics
        prv_record_wifi_connection_metrics(iface);
      }
    } break;

    case NET_EVENT_WIFI_DISCONNECT_RESULT: {
      const struct wifi_status *status = (const struct wifi_status *)cb->info;
      if (status->status) {
        // TODO: Record disconnect error
      } else {
        // stop connected-time timer, increment disconnect count
        MEMFAULT_METRIC_TIMER_STOP(wifi_connected_time_ms);
        MEMFAULT_METRIC_ADD(wifi_disconnect_count, 1);
      }
    } break;

    default:
      // shouldn't happen since this callback only subscribes to the above events
      break;
  }
}

static int prv_init_wifi_metrics(void) {
  static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
  net_mgmt_init_event_callback(&wifi_shell_mgmt_cb, prv_wifi_event_callback,
                               // TODO: NET_EVENT_L4_CONNECTED + NET_EVENT_L4_DISCONNECTED instead?
                               NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);

  net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

  return 0;
}
SYS_INIT(prv_init_wifi_metrics, APPLICATION, CONFIG_MEMFAULT_INIT_PRIORITY);
