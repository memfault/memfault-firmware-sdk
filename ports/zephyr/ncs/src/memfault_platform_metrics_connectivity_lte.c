//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!

#include "memfault/ports/ncs/version.h"

//! In NCS < 2.0, lte_lc.h is missing integer type definitions, so we need to include them before
//! lte_lc.h
#if !MEMFAULT_NCS_VERSION_GT(1, 9)
  #include <stddef.h>
  #include <stdint.h>
#endif

#ifdef CONFIG_LTE_LINK_CONTROL
  #include <modem/lte_lc.h>
#else
  //! CFUN mode values per 3GPP TS 27.007; mirrors lte_lc_func_mode enum values
  #define LTE_LC_FUNC_MODE_POWER_OFF 0
  #define LTE_LC_FUNC_MODE_NORMAL 1
  #define LTE_LC_FUNC_MODE_OFFLINE 4
  #define LTE_LC_FUNC_MODE_DEACTIVATE_LTE 20
  #define LTE_LC_FUNC_MODE_ACTIVATE_LTE 21
  #include <modem/at_monitor.h>
  #include <nrf_modem_at.h>
  #include <stdio.h>
#endif

#include <modem/nrf_modem_lib.h>

#include "memfault/core/debug_log.h"
#include "memfault/metrics/connectivity.h"
#include "memfault/metrics/platform/connectivity.h"

#ifdef CONFIG_LTE_LINK_CONTROL

//! Handler for LTE events
static void prv_memfault_lte_event_handler(const struct lte_lc_evt *const evt) {
  switch (evt->type) {
    case LTE_LC_EVT_NW_REG_STATUS:
      switch (evt->nw_reg_status) {
        case LTE_LC_NW_REG_REGISTERED_HOME:
          // intentional fallthrough
        case LTE_LC_NW_REG_REGISTERED_ROAMING:
          MEMFAULT_LOG_DEBUG("Connected state: connected");
          memfault_metrics_connectivity_connected_state_change(
            kMemfaultMetricsConnectivityState_Connected);
          break;
        case LTE_LC_NW_REG_NOT_REGISTERED:
          // intentional fallthrough
        case LTE_LC_NW_REG_SEARCHING:
          // intentional fallthrough
        case LTE_LC_NW_REG_REGISTRATION_DENIED:
          // intentional fallthrough
        case LTE_LC_NW_REG_UNKNOWN:
          // intentional fallthrough
        case LTE_LC_NW_REG_UICC_FAIL:
          MEMFAULT_LOG_DEBUG("Connected state: connection lost");
          memfault_metrics_connectivity_connected_state_change(
            kMemfaultMetricsConnectivityState_ConnectionLost);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

#else  // !CONFIG_LTE_LINK_CONTROL

  //! +CEREG registration status values per 3GPP TS 27.007
  #define CEREG_STAT_HOME 1
  #define CEREG_STAT_ROAMING 5

static void prv_update_cereg_stat(int stat) {
  if (stat == CEREG_STAT_HOME || stat == CEREG_STAT_ROAMING) {
    MEMFAULT_LOG_INFO("Connected state: connected");
    memfault_metrics_connectivity_connected_state_change(
      kMemfaultMetricsConnectivityState_Connected);
  } else {
    MEMFAULT_LOG_INFO("Connected state: connection lost");
    memfault_metrics_connectivity_connected_state_change(
      kMemfaultMetricsConnectivityState_ConnectionLost);
  }
}

//! AT monitor handler for +CEREG unsolicited result codes.
//! Notif format: "+CEREG: <stat>" or "+CEREG: <n>,<stat>[,<tac>,<ci>[,<AcT>]]"
static void prv_cereg_handler(const char *notif) {
  int n = 0, stat = 0;

  //! Try "<n>,<stat>" format first (response to AT+CEREG?), fall back to "<stat>" (URC)
  if (sscanf(notif, "+CEREG: %d,%d", &n, &stat) < 2) {
    if (sscanf(notif, "+CEREG: %d", &stat) < 1) {
      return;
    }
  }

  prv_update_cereg_stat(stat);
}

AT_MONITOR(memfault_cereg_monitor, "CEREG", prv_cereg_handler);

//! +CFUN mode change handler for started/stopped state tracking.
//! Uses AT_MONITOR so it fires for any AT+CFUN change, including raw AT commands.
static void prv_cfun_handler(const char *notif) {
  int mode;
  if (sscanf(notif, "+CFUN: %d", &mode) != 1) {
    return;
  }
  switch (mode) {
    case LTE_LC_FUNC_MODE_NORMAL:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
      MEMFAULT_LOG_INFO("Connected state: started");
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Started);
      break;
    case LTE_LC_FUNC_MODE_POWER_OFF:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_OFFLINE:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
      MEMFAULT_LOG_INFO("Connected state: stopped");
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Stopped);
      break;
    default:
      break;
  }
}

AT_MONITOR(memfault_cfun_monitor, "CFUN", prv_cfun_handler);

//! Subscribe to +CEREG notifications immediately after modem library initializes so the
//! subscription is active before AT+CFUN=1 is called, regardless of how CFUN is set.
static void prv_cereg_on_modem_init(int err, void *ctx) {
  ARG_UNUSED(ctx);
  if (err != 0) {
    return;
  }
  nrf_modem_at_printf("AT+CEREG=1");
  //! Read current state in case the modem is already registered (e.g. warm reboot)
  int cereg_n, cereg_stat;
  if (nrf_modem_at_scanf("AT+CEREG?", "+CEREG: %d,%d", &cereg_n, &cereg_stat) == 2) {
    prv_update_cereg_stat(cereg_stat);
  }
}

NRF_MODEM_LIB_ON_INIT(memfault_cereg_init_hook, prv_cereg_on_modem_init, NULL);

#endif  // CONFIG_LTE_LINK_CONTROL

//! The LTE_LC_ON_CFUN macro was introduced in nRF Connect SDK 2.0.
//! For NCS 2.8+, NRF_MODEM_LIB_ON_CFUN is used instead and does not require LTE_LINK_CONTROL.
//! For NCS 2.0-2.7 without LTE_LINK_CONTROL, mode change tracking is unavailable.
#ifdef CONFIG_LTE_LINK_CONTROL
  #if MEMFAULT_NCS_VERSION_GT(1, 9)
    #if MEMFAULT_NCS_VERSION_GT(2, 7)
static void prv_memfault_lte_mode_cb(int mode, MEMFAULT_UNUSED void *ctx) {
    #else
static void prv_memfault_lte_mode_cb(enum lte_lc_func_mode mode, MEMFAULT_UNUSED void *ctx) {
    #endif
  switch (mode) {
    case LTE_LC_FUNC_MODE_NORMAL:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
      MEMFAULT_LOG_DEBUG("Connected state: started");
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Started);
      break;
    case LTE_LC_FUNC_MODE_POWER_OFF:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_OFFLINE:
      // intentional fallthrough
    case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
      MEMFAULT_LOG_DEBUG("Connected state: stopped");
      memfault_metrics_connectivity_connected_state_change(
        kMemfaultMetricsConnectivityState_Stopped);
      break;
    default:
      break;
  }
}
    #if MEMFAULT_NCS_VERSION_GT(2, 7)
NRF_MODEM_LIB_ON_CFUN(memfault_lte_mode_cb, prv_memfault_lte_mode_cb, NULL);
    #else
LTE_LC_ON_CFUN(memfault_lte_mode_cb, prv_memfault_lte_mode_cb, NULL);
    #endif
  #endif  // MEMFAULT_NCS_VERSION_GT(1, 9)
#endif    // CONFIG_LTE_LINK_CONTROL

void memfault_platform_metrics_connectivity_boot(void) {
#ifdef CONFIG_LTE_LINK_CONTROL
  lte_lc_register_handler(prv_memfault_lte_event_handler);
#endif
  //! Without LTE_LINK_CONTROL: +CEREG is enabled via the AT_MONITOR CFUN handler
}
