//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

// clang-format off
#include "memfault/ports/zephyr/include_compatibility.h"

#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(bluetooth/bluetooth.h)
#include MEMFAULT_ZEPHYR_INCLUDE(bluetooth/conn.h)
#include MEMFAULT_ZEPHYR_INCLUDE(bluetooth/gatt.h)
#include MEMFAULT_ZEPHYR_INCLUDE(bluetooth/hci.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/byteorder.h)

// clang-format on

#include <stdbool.h>
#include <stdio.h>

#include "memfault/core/debug_log.h"
#include "memfault/core/platform/core.h"
#include "memfault/metrics/metrics.h"
#include "memfault/metrics/platform/timer.h"
#include "memfault/ports/zephyr/version.h"

#define BT_LE_INTERVAL_UNIT_US  (1250U)

static struct bt_conn *s_mflt_bt_current_conn = NULL;
static uint32_t s_mflt_bt_connection_interval_us = 0;
struct k_work_delayable s_mflt_bt_delayed_metrics_work;

static void prv_record_gatt_mtu(struct bt_conn *conn) {
  uint16_t mtu = bt_gatt_get_mtu(conn);
  MEMFAULT_METRIC_SET_UNSIGNED(bt_gatt_mtu_size, mtu);
}

static void prv_count_connection_events(uint32_t interval_us, bool reset_time) {
  // to accumulate data correctly on:
  // - connection interval change
  // - connection up/down
  //
  // keep track of the last time the measurement was taken
  static uint64_t s_last_measurement_time_ms = 0;

  // if resetting tracked time, reset and return
  if (reset_time) {
    s_last_measurement_time_ms = memfault_platform_get_time_since_boot_ms();
    return;
  }

  if (interval_us) {
    // calculate events per second: 1000000us / interval_us
    int32_t events_per_second = 1000000 / interval_us;

    // compute connection events accumulated
    const uint64_t current_time_ms = memfault_platform_get_time_since_boot_ms();
    const int32_t connection_events =
      events_per_second * (int32_t)(current_time_ms - s_last_measurement_time_ms) / 1000;
    MEMFAULT_METRIC_ADD(bt_connection_event_count, connection_events);
    s_last_measurement_time_ms = current_time_ms;
  }
}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
static void prv_record_connection_rssi(struct bt_conn *conn) {
  // Try to get RSSI using vendor-specific command
  // Note: This API may not be available on all platforms
  uint16_t hci_conn_handle;
  int err = bt_hci_get_conn_handle(s_mflt_bt_current_conn, &hci_conn_handle);
  if (err) {
    MEMFAULT_LOG_ERROR("Failed to get RSSI connection handle: %d", err);
    return;
  }

  struct bt_hci_cp_read_rssi *cp;

  #if MEMFAULT_ZEPHYR_VERSION_GT(4, 1)
  struct net_buf *buf = bt_hci_cmd_alloc(K_FOREVER);
  #else
  struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
  #endif

  if (!buf) {
    return;
  }

  cp = net_buf_add(buf, sizeof(*cp));
  cp->handle = sys_cpu_to_le16(hci_conn_handle);

  struct net_buf *rsp = NULL;
  err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
  if (err) {
    MEMFAULT_LOG_ERROR("Read RSSI err: %d\n", err);
  } else {
    struct bt_hci_rp_read_rssi *rp = (void *)rsp->data;
    int8_t rssi = rp->rssi;

    MEMFAULT_METRIC_SET_SIGNED(bt_connection_rssi, rssi);
  }

  net_buf_unref(rsp);
}
#else
static void prv_record_connection_rssi(struct bt_conn *conn) {
  // RSSI reading not available on this platform configuration
  (void)conn;
}
#endif  // defined(CONFIG_BT_CTLR_CONN_RSSI)

static void prv_delayed_metrics_work_handler(struct k_work *work) {
  (void)work;

  // if connection has gone down before this runs, just return
  if (!s_mflt_bt_current_conn) {
    return;
  }

  // Read RSSI now
  prv_record_connection_rssi(s_mflt_bt_current_conn);

#if defined(CONFIG_BT_REMOTE_INFO)
  char remote_info_str[32] = { 0 };
  struct bt_conn_remote_info remote_info;

  int err = bt_conn_get_remote_info(s_mflt_bt_current_conn, &remote_info);

  if (err) {
    MEMFAULT_LOG_ERROR("Failed to get remote info: %d", err);
  } else {
    snprintf(remote_info_str, sizeof(remote_info_str), "%02x:%04x.%04x", remote_info.version,
             remote_info.manufacturer, remote_info.subversion);
    MEMFAULT_METRIC_SET_STRING(bt_connection_remote_info, remote_info_str);
  }
#endif  // defined(CONFIG_BT_REMOTE_INFO)
}

//! Read connection params, and update connection interval for later usage
static void prv_record_connection_params(struct bt_conn *conn) {
  struct bt_conn_info info;
  if (bt_conn_get_info(conn, &info) == 0) {
    MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_interval_us, info.le.interval_us);
    MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_latency, info.le.latency);
    MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_timeout, info.le.timeout);
#if (defined(MEMFAULT_ZEPHYR_VERSION_GT) && MEMFAULT_ZEPHYR_VERSION_GT(4, 3)) || \
    (defined(MEMFAULT_NCS_VERSION_GT) && MEMFAULT_NCS_VERSION_GT(3, 1))
    s_mflt_bt_connection_interval_us = info.le.interval_us;
#else
    s_mflt_bt_connection_interval_us = info.le.interval * BT_LE_INTERVAL_UNIT_US;
#endif
  } else {
    MEMFAULT_LOG_ERROR("Failed to get connection info");
    s_mflt_bt_connection_interval_us = 0;
  }
}

static void prv_bt_connected_cb(struct bt_conn *conn, uint8_t err) {
  if (err) {
    return;
  }

  s_mflt_bt_current_conn = bt_conn_ref(conn);

  // Start connected time timer
  MEMFAULT_METRIC_TIMER_START(bt_connected_time_ms);

  // Record connection metrics
  prv_record_gatt_mtu(conn);
  prv_record_connection_params(conn);

  // Start a one-shot timer to read additional metrics after a delay
  k_work_schedule(&s_mflt_bt_delayed_metrics_work,
                  K_MSEC(CONFIG_MEMFAULT_METRICS_BLUETOOTH_DELAYED_MS));

  // Reset connection event count timer
  prv_count_connection_events(0, true);
}

static void prv_bt_disconnected_cb(struct bt_conn *conn, uint8_t reason) {
  // tally connection events
  prv_count_connection_events(s_mflt_bt_connection_interval_us, false);
  s_mflt_bt_connection_interval_us = 0;

  if (s_mflt_bt_current_conn == conn) {
    bt_conn_unref(s_mflt_bt_current_conn);
    s_mflt_bt_current_conn = NULL;
  }

  // Increment disconnect counter
  MEMFAULT_METRIC_ADD(bt_disconnect_count, 1);

  // Stop connected time timer
  MEMFAULT_METRIC_TIMER_STOP(bt_connected_time_ms);
}

#if defined(CONFIG_BT_REMOTE_INFO)
// Note: this does not seem to fire when the remote initiates the connection,
// only when this device does.
static void prv_record_remote_info_cb(struct bt_conn *conn,
                                      struct bt_conn_remote_info *remote_info) {
  // Record remote device information
  char remote_info_str[32] = { 0 };

  snprintf(remote_info_str, sizeof(remote_info_str), "%02x:%04x.%04x", remote_info->version,
           remote_info->manufacturer, remote_info->subversion);
  MEMFAULT_METRIC_SET_STRING(bt_connection_remote_info, remote_info_str);
}
#endif  // defined(CONFIG_BT_REMOTE_INFO)

static void prv_bt_le_param_updated_cb(struct bt_conn *conn, uint16_t interval, uint16_t latency,
                                       uint16_t timeout) {
  uint32_t interval_us = interval * BT_LE_INTERVAL_UNIT_US;

  // Record LE connection parameters
  MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_interval_us, interval_us);
  MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_latency, latency);
  MEMFAULT_METRIC_SET_UNSIGNED(bt_connection_timeout, timeout);

  // Tally connection event counts received with the previous interval setting
  prv_count_connection_events(s_mflt_bt_connection_interval_us, false);

  // Update connection interval for computing connection event count
  s_mflt_bt_connection_interval_us = interval_us;
}

BT_CONN_CB_DEFINE(bt_metrics_conn_callbacks) = {
  .connected = prv_bt_connected_cb,
  .disconnected = prv_bt_disconnected_cb,
  .le_param_updated = prv_bt_le_param_updated_cb,
#if defined(CONFIG_BT_REMOTE_INFO)
  .remote_info_available = prv_record_remote_info_cb,
#endif
};

static void prv_bt_att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
  // note: the BT spec requires a symmetric tx/rx MTU size
  // see reference:
  // https://github.com/zephyrproject-rtos/zephyr/blob/v4.2.0/subsys/bluetooth/host/att.c#L136-L144
  MEMFAULT_METRIC_SET_UNSIGNED(bt_gatt_mtu_size, MIN(tx, rx));
}

static struct bt_gatt_cb prv_gatt_callbacks = {
  .att_mtu_updated = prv_bt_att_mtu_updated,
};

void memfault_bluetooth_metrics_heartbeat_update(void) {
  if (s_mflt_bt_current_conn != NULL) {
    // Update connection event count estimate
    prv_count_connection_events(s_mflt_bt_connection_interval_us, false);

    // Record current RSSI (if available)
    prv_record_connection_rssi(s_mflt_bt_current_conn);

    // And connection parameters + MTU. Callbacks should refresh these on
    // change, but we want to report them if there was no change too.
    prv_record_connection_params(s_mflt_bt_current_conn);
    prv_record_gatt_mtu(s_mflt_bt_current_conn);
  }
}

static int prv_init_bluetooth_metrics(void) {
  k_work_init_delayable(&s_mflt_bt_delayed_metrics_work, prv_delayed_metrics_work_handler);

  bt_gatt_cb_register(&prv_gatt_callbacks);
  return 0;
}
SYS_INIT(prv_init_bluetooth_metrics, APPLICATION, CONFIG_MEMFAULT_INIT_PRIORITY);
