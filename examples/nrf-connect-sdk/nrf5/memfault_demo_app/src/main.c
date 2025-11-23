/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/mds.h>
#include <dk_buttons_and_leds.h>
#include <hw_id.h>
#include <memfault/components.h>
#include <memfault/core/trace_event.h>
#include <memfault/metrics/metrics.h>
#include <memfault/ports/watchdog.h>
#include <memfault_ncs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2

#define RUN_LED_BLINK_INTERVAL 1000

static const struct bt_data ad[] = {
  BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
  BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_MDS_VAL),
};

static const struct bt_data sd[] = {
  BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_conn *mds_conn;
static struct k_work adv_work;

static void bas_work_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(bas_work, bas_work_handler);

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  if (!err) {
    printk("Security changed: %s level %u\n", addr, level);
  } else {
    printk("Security failed: %s level %u err %d %s\n", addr, level, err,
           bt_security_err_to_str(err));
  }

  if (level >= BT_SECURITY_L2) {
    if (!mds_conn) {
      mds_conn = conn;
    }
  }
}

static void adv_work_handler(struct k_work *work) {
  int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
    return;
  }

  printk("Advertising successfully started\n");
}

static void advertising_start(void) {
  k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t conn_err) {
  char addr[BT_ADDR_LE_STR_LEN];

  if (conn_err) {
    printk("Connection failed, err 0x%02x %s\n", conn_err, bt_hci_err_to_str(conn_err));
    return;
  }

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  printk("Connected %s\n", addr);

  dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

  dk_set_led_off(CON_STATUS_LED);

  if (conn == mds_conn) {
    mds_conn = NULL;
  }
}

static void recycled_cb(void) {
  printk("Connection object available from previous conn. Disconnect is complete!\n");
  advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
  .connected = connected,
  .disconnected = disconnected,
  .security_changed = security_changed,
  .recycled = recycled_cb,
};

static void pairing_complete(struct bt_conn *conn, bool bonded) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing failed conn: %s, reason %d %s\n", addr, reason, bt_security_err_to_str(reason));
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = { .pairing_complete =
                                                                  pairing_complete,
                                                                .pairing_failed = pairing_failed };

static void auth_cancel(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];

  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
  .cancel = auth_cancel,
};

static bool mds_access_enable(struct bt_conn *conn) {
  if (mds_conn && (conn == mds_conn)) {
    return true;
  }

  return false;
}

static const struct bt_mds_cb mds_cb = {
  .access_enable = mds_access_enable,
};

static void button_handler(uint32_t button_state, uint32_t has_changed) {
  static bool time_measure_start;

  int err;
  uint32_t buttons = button_state & has_changed;

  if (buttons & DK_BTN1_MSK) {
    time_measure_start = !time_measure_start;

    if (time_measure_start) {
      err = MEMFAULT_METRIC_TIMER_START(button_elapsed_time_ms);
      if (err) {
        printk("Failed to start memfault metrics timer: %d\n", err);
      }
    } else {
      err = MEMFAULT_METRIC_TIMER_STOP(button_elapsed_time_ms);
      if (err) {
        printk("Failed to stop memfault metrics: %d\n", err);
      }

      /* Trigger collection of heartbeat data. */
      memfault_metrics_heartbeat_debug_trigger();
    }
  }

  if (has_changed & DK_BTN2_MSK) {
    bool button_state = (buttons & DK_BTN2_MSK) ? 1 : 0;

    MEMFAULT_TRACE_EVENT_WITH_LOG(button_state_changed, "Button state: %u", button_state);

    printk("button_state_changed event has been tracked, button state: %u\n", button_state);
  }

  if (buttons & DK_BTN3_MSK) {
    err = MEMFAULT_METRIC_ADD(button_press_count, 1);
    if (err) {
      printk("Failed to increase button_press_count metric: %d\n", err);
    } else {
      printk("button_press_count metric increased\n");
    }
  }

  if (buttons & DK_BTN4_MSK) {
    volatile uint32_t i;

    printk("Division by zero will now be triggered\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
    i = 1 / 0;
#pragma GCC diagnostic pop
    ARG_UNUSED(i);
  }
}

static void bas_notify(void) {
  int err;
  uint8_t battery_level = bt_bas_get_battery_level();

  __ASSERT_NO_MSG(battery_level > 0);

  battery_level--;

  if (battery_level == 0) {
    battery_level = 100U;
  }

  err = MEMFAULT_METRIC_SET_UNSIGNED(battery_soc_pct, battery_level);
  if (err) {
    printk("Failed to set battery_soc_pct memfault metrics (err %d)\n", err);
  }

  bt_bas_set_battery_level(battery_level);
}

static void bas_work_handler(struct k_work *work) {
  bas_notify();
  k_work_reschedule((struct k_work_delayable *)work, K_SECONDS(1));
}

#define SERIAL_NUMBER_SETTING_KEY "bt/dis/serial"

static void serial_number_init(void) {
  int err = settings_get_val_len(SERIAL_NUMBER_SETTING_KEY);

  printk("Checked for existing serial number in settings, length %d\n", err);

  /* Check if the serial number is already set */
  char serial_buf[64] = "unknown";
  int serial_len = settings_load_one(SERIAL_NUMBER_SETTING_KEY, serial_buf, sizeof(serial_buf));

  if (serial_len == 0) {
    printk("📥 Writing new device serial number to " SERIAL_NUMBER_SETTING_KEY " from HW info\n");
    int ret = hw_id_get(serial_buf, sizeof(serial_buf));

    if (ret) {
      printk("Failed to get device ID from HW ID (err %d)\n", ret);
      return;
    }
    serial_len = strlen(serial_buf);
    settings_save_one(SERIAL_NUMBER_SETTING_KEY, serial_buf, serial_len);
    ret = settings_runtime_set(SERIAL_NUMBER_SETTING_KEY, serial_buf, serial_len);
    if (ret) {
      printk("Failed to set device serial number in settings runtime (err %d)\n", ret);
    }
  } else {
    printk("📤 Loaded serial number from settings\n");
  }

  printk("➡️ Device serial set to: %s\n", serial_buf);

  // Apply the serial number to Memfault
  (void)memfault_ncs_device_id_set(serial_buf, serial_len);
}

// Shell command to set Bluetooth DIS + Memfault serial number
static int cmd_set_bt_serial(const struct shell *shell_ptr, size_t argc, char *argv[]) {
  if (argc != 2) {
    shell_print(shell_ptr, "Usage: set-bt-serial <serial_number>");
    return -EINVAL;
  }

  const char *serial_number = argv[1];

  int ret = settings_save_one(SERIAL_NUMBER_SETTING_KEY, serial_number, strlen(serial_number));
  if (ret) {
    shell_print(shell_ptr, "Error: Failed to save serial number to settings (err %d)", ret);
    return ret;
  }
  ret = settings_runtime_set(SERIAL_NUMBER_SETTING_KEY, serial_number, strlen(serial_number));
  if (ret) {
    shell_print(shell_ptr, "Error: Failed to set serial number in settings runtime (err %d)", ret);
    return ret;
  }
  (void)memfault_ncs_device_id_set(serial_number, strlen(serial_number));

  shell_print(shell_ptr, "Bluetooth serial number saved to settings successfully");
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config,
                               SHELL_CMD_ARG(set_bt_serial, NULL,
                                             "Set Bluetooth serial number in settings",
                                             cmd_set_bt_serial, 2, 0),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(config, &sub_config, "Configure the example", NULL);

#if CONFIG_MEMFAULT_APP_CAPTURE_ALL_RAM
// capture *ALL* of ram
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  static sMfltCoredumpRegion s_regions[] = {
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(CONFIG_PM_SRAM_BASE, CONFIG_PM_SRAM_SIZE),
  };
  *num_regions = ARRAY_SIZE(s_regions);
  return s_regions;
}
#endif  // CONFIG_MEMFAULT_APP_CAPTURE_ALL_RAM

#define WD_FEED_THREAD_STACK_SIZE 500
// set priority to lowest application thread; shell_uart, where the 'mflt test
// hang' command runs from, uses the same priority by default, so this should
// not preempt it and correctly trip the watchdog
#if CONFIG_SHELL_THREAD_PRIORITY_OVERRIDE
  #error "Watchdog feed thread priority must be lower than shell thread priority"
#endif
#define WD_FEED_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static void prv_wd_feed_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  while (1) {
    memfault_software_watchdog_feed();
    k_sleep(K_SECONDS(1));
  }
}
K_THREAD_DEFINE(wd_feed_thread, WD_FEED_THREAD_STACK_SIZE, prv_wd_feed_thread_function, NULL, NULL,
                NULL, WD_FEED_THREAD_PRIORITY, 0, 0);

static void prv_start_watchdog_feed_thread(void) {
  memfault_software_watchdog_enable();
  k_thread_name_set(wd_feed_thread, "wd_feed_thread");
  k_thread_start(wd_feed_thread);
}

int main(void) {
  uint32_t blink_status = 0;
  int err;

  printk(MEMFAULT_BANNER_COLORIZED "\n");
  printk("Memfault Demo Application\n");
  printk("Board: " CONFIG_BOARD "\n");

  prv_start_watchdog_feed_thread();

  err = dk_leds_init();
  if (err) {
    printk("LEDs init failed (err %d)\n", err);
    return 0;
  }

  err = dk_buttons_init(button_handler);
  if (err) {
    printk("Failed to initialize buttons (err %d)\n", err);
    return 0;
  }

  err = bt_mds_cb_register(&mds_cb);
  if (err) {
    printk("Memfault Diagnostic service callback registration failed (err %d)\n", err);
    return 0;
  }

  err = bt_enable(NULL);
  if (err) {
    printk("Bluetooth init failed (err %d)\n", err);
    return 0;
  }

  err = bt_conn_auth_cb_register(&conn_auth_callbacks);
  if (err) {
    printk("Failed to register authorization callbacks (err %d)\n", err);
    return 0;
  }

  err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
  if (err) {
    printk("Failed to register authorization info callbacks (err %d)\n", err);
    return 0;
  }

  printk("Bluetooth initialized\n");

  if (IS_ENABLED(CONFIG_SETTINGS)) {
    err = settings_load();
    if (err) {
      printk("Failed to load settings (err %d)\n", err);
      return 0;
    }
    serial_number_init();
  }

  k_work_init(&adv_work, adv_work_handler);
  advertising_start();

  k_work_schedule(&bas_work, K_SECONDS(1));

  for (;;) {
    dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
    k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
  }
}
