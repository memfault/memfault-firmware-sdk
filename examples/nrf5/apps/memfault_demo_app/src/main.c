//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A minimal demo app for the NRF52 using the nRF5 v15.2 SDK that allows for crashes to be generated
//! from a RTT CLI and coredumps to be saved

#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_timer.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrfx_wdt.h"

#include "mflt_cli.h"

#include "memfault/components.h"
#include "memfault/ports/watchdog.h"

static void timers_init(void) {
  ret_code_t err_code;

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
}

static void log_init(void) {
  const ret_code_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void prv_wdt_event_handler(void) {
  // Note: This handler should be entirely unreachable as the software watchdog should kick in
  // first but just in case let's reboot if it is invoked.
  memfault_reboot_tracking_mark_reset_imminent(kMfltRebootReason_HardwareWatchdog, NULL);
  memfault_platform_reboot();
  MEMFAULT_UNREACHABLE;
}

static nrfx_wdt_channel_id s_wdt_channel_id;

static void prv_hardware_watchdog_enable(void) {
  nrfx_wdt_config_t config = NRFX_WDT_DEAFULT_CONFIG;

  uint32_t err_code = nrfx_wdt_init(&config, prv_wdt_event_handler);
  APP_ERROR_CHECK(err_code);
  err_code = nrfx_wdt_channel_alloc(&s_wdt_channel_id);
  APP_ERROR_CHECK(err_code);
  nrfx_wdt_enable();
}

static void prv_hardware_watchdog_feed(void) {
  nrfx_wdt_channel_feed(s_wdt_channel_id);
}

int main(void) {
  nrf_drv_clock_init();
  nrf_drv_clock_lfclk_request(NULL);

  log_init();
  timers_init();
  mflt_cli_init();
  memfault_platform_boot();
  prv_hardware_watchdog_enable();
  memfault_software_watchdog_enable();

  while (1) {
    prv_hardware_watchdog_feed();
    memfault_software_watchdog_feed();

    mflt_cli_try_process();
    if (NRF_LOG_PROCESS() == false) {
      nrf_pwr_mgmt_run();
    }
  }
}
