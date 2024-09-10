//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "memfault/ports/ncs/version.h"

//! In NCS < 2.0, lte_lc.h is missing integer type definitions, so we need to include them before
//! lte_lc.h
#if !MEMFAULT_NCS_VERSION_GT(1, 9)
  #include <stddef.h>
  #include <stdint.h>
#endif

#include <modem/lte_lc.h>

#include "memfault/core/debug_log.h"
#include "memfault/metrics/connectivity.h"
#include "memfault/metrics/platform/connectivity.h"

#if defined(CONFIG_MEMFAULT_NRF_CONNECTIVITY_CONNECTED_TIME_NRF91X)
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

//! The LTE_LC_ON_CFUN macro was introduced in nRF Connect SDK 2.0
  #if MEMFAULT_NCS_VERSION_GT(2, 0)
//! Callback for LTE mode changes
static void prv_memfault_lte_mode_cb(enum lte_lc_func_mode mode, MEMFAULT_UNUSED void *ctx) {
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

LTE_LC_ON_CFUN(memfault_lte_mode_cb, prv_memfault_lte_mode_cb, NULL);
  #endif /* MEMFAULT_VERSION_GT */

#endif /* CONFIG_MEMFAULT_NRF_CONNECTIVITY_CONNECTED_TIME_NRF91X */

void memfault_platform_metrics_connectivity_boot(void) {
#if defined(CONFIG_MEMFAULT_NRF_CONNECTIVITY_CONNECTED_TIME_NRF91X)
  lte_lc_register_handler(prv_memfault_lte_event_handler);
#endif /* CONFIG_MEMFAULT_NRF_CONNECTIVITY_CONNECTED_TIME_NRF91X */
}
