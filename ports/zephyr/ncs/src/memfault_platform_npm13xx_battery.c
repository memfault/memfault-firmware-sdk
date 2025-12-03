//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include MEMFAULT_ZEPHYR_INCLUDE(devicetree.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/sensor.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/sensor/npm13xx_charger.h)
#include MEMFAULT_ZEPHYR_INCLUDE(drivers/mfd/npm13xx.h)
#include "memfault/components.h"
#include "memfault_nrf_platform_battery_model.h"
#include "nrf_fuel_gauge.h"

// nPM13xx BCHGCHARGESTATUS register mask definitions, compatible with both nPM1300 and nPM1304
// See https://docs-be.nordicsemi.com/bundle/ps_npm1300/page/nPM1300_PS_v1.2.pdf and
// https://docs-be.nordicsemi.com/bundle/ps_npm1304/page/pdf/nPM1304_Preliminary_Datasheet_v0.7.pdf
#define NPM13XX_CHG_STATUS_TC_MASK BIT(2)
#define NPM13XX_CHG_STATUS_CC_MASK BIT(3)
#define NPM13XX_CHG_STATUS_CV_MASK BIT(4)

#if defined(CONFIG_DT_HAS_NORDIC_NPM1300_CHARGER_ENABLED)
static const struct device *s_npm13xx_dev = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));
#elif defined(CONFIG_DT_HAS_NORDIC_NPM1304_CHARGER_ENABLED)
static const struct device *s_npm13xx_dev = DEVICE_DT_GET(DT_NODELABEL(npm1304_charger));
#else
  #error \
    "Unsupported nPM13xx charger device or device not enabled in devicetree. Contact mflt.io/contact-support for assistance."
#endif

static int64_t s_ref_time;

static int prv_npm13xx_read_sensors(float *voltage, float *current, float *temp,
                                    int *charging_status) {
  struct sensor_value reading;
  int err;

  err = sensor_sample_fetch(s_npm13xx_dev);
  if (err < 0) {
    return err;
  }

  err = sensor_channel_get(s_npm13xx_dev, SENSOR_CHAN_GAUGE_VOLTAGE, &reading);
  if (err) {
    return err;
  }
  *voltage = sensor_value_to_float(&reading);

  err = sensor_channel_get(s_npm13xx_dev, SENSOR_CHAN_GAUGE_TEMP, &reading);
  if (err) {
    return err;
  }
  *temp = sensor_value_to_float(&reading);

  err = sensor_channel_get(s_npm13xx_dev, SENSOR_CHAN_GAUGE_AVG_CURRENT, &reading);
  if (err) {
    return err;
  }

  // Zephyr sensor API returns current as negative for discharging, positive for charging
  // but nRF fuel gauge library expects opposite. Flip here for uniformity
  *current = -sensor_value_to_float(&reading);

  // optionally read charging status
  if (charging_status == NULL) {
    return 0;
  }
  err = sensor_channel_get(s_npm13xx_dev, (enum sensor_channel)SENSOR_CHAN_NPM13XX_CHARGER_STATUS,
                           &reading);
  if (err) {
    return err;
  }
  *charging_status = reading.val1;

  return 0;
}

static bool prv_npm13xx_is_discharging(int charging_status) {
  return (charging_status & (NPM13XX_CHG_STATUS_TC_MASK | NPM13XX_CHG_STATUS_CC_MASK |
                             NPM13XX_CHG_STATUS_CV_MASK)) == 0;
}

int memfault_platform_get_stateofcharge(sMfltPlatformBatterySoc *soc) {
  // Float type required to retain precision when passing units of volts, amps, and degrees C to the
  // nRF fuel gauge API
  float voltage, current, temp;
  int charging_status;
  int err = prv_npm13xx_read_sensors(&voltage, &current, &temp, &charging_status);
  if (err < 0) {
    MEMFAULT_LOG_ERROR("Failure reading charger sensors, error: %d", err);
    return -1;
  }

  int64_t time_delta_ms = k_uptime_delta(&s_ref_time);
  float time_delta_s = (float)time_delta_ms / 1000.0f;
  struct nrf_fuel_gauge_state_info info;
  nrf_fuel_gauge_process(voltage, current, temp, time_delta_s, &info);

  // soc_raw is already 0-100, so scale only by scale value
  soc->soc = (uint32_t)(info.soc_raw * (float)CONFIG_MEMFAULT_METRICS_BATTERY_SOC_PCT_SCALE_VALUE);
  soc->discharging = prv_npm13xx_is_discharging(charging_status);

  // Scale by scale value (must match definition in memfault_metrics_heartbeat_ncs_port_config.def)
  uint32_t voltage_scaled = (uint32_t)(voltage * 1000.0f);
  MEMFAULT_METRIC_SET_UNSIGNED(battery_voltage, voltage_scaled);

  MEMFAULT_LOG_DEBUG("Battery SOC %u.%03u%%, discharging=%d, voltage=%d mv", soc->soc / 1000,
                     soc->soc % 1000, soc->discharging, (int)voltage_scaled);

  s_ref_time = k_uptime_get();
  return 0;
}

static int prv_platform_battery_init(void) {
  if (!device_is_ready(s_npm13xx_dev)) {
    printk("Charger device not ready\n");
    return -1;
  }

  struct nrf_fuel_gauge_init_parameters parameters = { .model = &battery_model };
  int err = prv_npm13xx_read_sensors(&parameters.v0, &parameters.i0, &parameters.t0, NULL);
  if (err < 0) {
    printk("Failure reading charger sensors, error: %d\n", err);
    return err;
  }

  err = nrf_fuel_gauge_init(&parameters, NULL);
  if (err) {
    printk("Failure initializing fuel gauge, error: %d\n", err);
    return err;
  }

  s_ref_time = k_uptime_get();
  return 0;
}

#if defined(CONFIG_MEMFAULT_INIT_LEVEL_POST_KERNEL)
  #error \
    "CONFIG_MEMFAULT_INIT_LEVEL_POST_KERNEL=y not supported with CONFIG_MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX. Please contact mflt.io/contact-support for assistance."
#elif (CONFIG_MEMFAULT_INIT_PRIORITY <= CONFIG_MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX_INIT_PRIORITY)
  #error \
    "MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX_INIT_PRIORITY must be set to a value less than MEMFAULT_INIT_PRIORITY (lower value = higher priority)"
#endif

SYS_INIT(prv_platform_battery_init,
#if defined(CONFIG_MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX_INIT_LEVEL_POST_KERNEL)
         POST_KERNEL,
#else
         APPLICATION,
#endif
         CONFIG_MEMFAULT_NRF_PLATFORM_BATTERY_NPM13XX_INIT_PRIORITY);
