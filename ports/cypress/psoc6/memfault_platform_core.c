//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Implementation of core dependencies and setup for PSOC6 based products that are using the
//! ModusToolbox SDK

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"
#include "memfault/ports/freertos.h"

#if MEMFAULT_PORT_MEMORY_TRACKING_ENABLED
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#include <malloc.h>
#endif
#endif /* MEMFAULT_PORT_MEMORY_TRACKING_ENABLED */

#if MEMFAULT_PORT_WIFI_TRACKING_ENABLED
#include "cy_wcm.h"
#endif /* MEMFAULT_PORT_WIFI_TRACKING_ENABLED */

#ifndef MEMFAULT_EVENT_STORAGE_RAM_SIZE
#define MEMFAULT_EVENT_STORAGE_RAM_SIZE 1024
#endif

#ifndef MEMFAULT_LOG_STORAGE_RAM_SIZE
  #define MEMFAULT_LOG_STORAGE_RAM_SIZE 512
#endif

#if MEMFAULT_PORT_WIFI_TRACKING_ENABLED
static cy_wcm_wlan_statistics_t s_last_wlan_statistics = { 0 };
#endif

void memfault_metrics_heartbeat_collect_sdk_data(void) {
#if MEMFAULT_PORT_MEMORY_TRACKING_ENABLED
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)

  struct mallinfo mall_info = mallinfo();

  // linker symbols for location of heap
  extern uint32_t __HeapBase;
  extern uint32_t __HeapLimit;

  const uint32_t heap_total_size = (uint32_t)&__HeapLimit - (uint32_t)&__HeapBase;

  MEMFAULT_METRIC_SET_UNSIGNED(Heap_TotalSize, heap_total_size);

  MEMFAULT_METRIC_SET_UNSIGNED(Heap_MinBytesFree, heap_total_size - mall_info.arena);

  MEMFAULT_METRIC_SET_UNSIGNED(Heap_BytesFree, heap_total_size - mall_info.uordblks);
  #endif /* defined(__GNUC__) && !defined(__ARMCC_VERSION) */
#endif /* MEMFAULT_PORT_MEMORY_TRACKING_ENABLED */

#if MEMFAULT_PORT_WIFI_TRACKING_ENABLED
  cy_wcm_associated_ap_info_t ap_info;
  cy_rslt_t result = cy_wcm_get_associated_ap_info(&ap_info);
  if (result == CY_RSLT_SUCCESS) {
    // Note: RSSI in dBm. (<-90=Very poor, >-30=Excellent)
    // A good way to determine whether or not issues an device is facing are due to poor signal strength
    MEMFAULT_METRIC_SET_SIGNED(Wifi_SignalStrength, ap_info.signal_strength);

    MEMFAULT_METRIC_SET_UNSIGNED(Wifi_Channel, ap_info.channel);
  }

  cy_wcm_wlan_statistics_t curr_stat;
  result = cy_wcm_get_wlan_statistics(CY_WCM_INTERFACE_TYPE_STA, &curr_stat);
  if (result == CY_RSLT_SUCCESS) {
    MEMFAULT_METRIC_SET_UNSIGNED(Wifi_TxBytes,
                                    curr_stat.tx_bytes - s_last_wlan_statistics.tx_bytes);
    MEMFAULT_METRIC_SET_UNSIGNED(Wifi_TxBytes,
                                    curr_stat.rx_bytes - s_last_wlan_statistics.rx_bytes);
    s_last_wlan_statistics = curr_stat;
  }
#endif /* MEMFAULT_PORT_WIFI_TRACKING_ENABLED */
}

#if MEMFAULT_PORT_WIFI_TRACKING_ENABLED

static void prv_wcm_event_cb(cy_wcm_event_t event, cy_wcm_event_data_t *event_data) {
  switch (event) {
    case CY_WCM_EVENT_CONNECTING:
      MEMFAULT_METRIC_TIMER_START(Wifi_ConnectingTime);
      break;

    case CY_WCM_EVENT_CONNECTED:
      MEMFAULT_METRIC_TIMER_STOP(Wifi_ConnectingTime);
      MEMFAULT_METRIC_TIMER_START(Wifi_ConnectedTime);
      MEMFAULT_METRIC_ADD(Wifi_ConnectCount, 1);
      break;

    case CY_WCM_EVENT_CONNECT_FAILED:
      MEMFAULT_METRIC_ADD(Wifi_ConnectFailureCount, 1);
      MEMFAULT_METRIC_TIMER_STOP(Wifi_ConnectingTime);
      MEMFAULT_METRIC_TIMER_STOP(Wifi_ConnectedTime);
      break;

    case CY_WCM_EVENT_RECONNECTED:
      MEMFAULT_METRIC_ADD(Wifi_ReconnectCount, 1);
      MEMFAULT_METRIC_TIMER_START(Wifi_ConnectedTime);
      break;

    case CY_WCM_EVENT_DISCONNECTED:
      MEMFAULT_METRIC_TIMER_STOP(Wifi_ConnectedTime);
      MEMFAULT_METRIC_ADD(Wifi_DisconnectCount, 1);
      break;

    case CY_WCM_EVENT_IP_CHANGED:
      break;

    case CY_WCM_EVENT_INITIATED_RETRY:
      break;

    case CY_WCM_EVENT_STA_JOINED_SOFTAP:
      break;

    case CY_WCM_EVENT_STA_LEFT_SOFTAP:
      break;

    default:
      break;
  }
}

void memfault_wcm_metrics_boot(void) {
  cy_wcm_register_event_callback(&prv_wcm_event_cb);
}

#endif /* MEMFAULT_PORT_WIFI_TRACKING_ENABLED */

MEMFAULT_WEAK
void memfault_platform_reboot(void) {
  NVIC_SystemReset();
  while (1) { } // unreachable
}

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    {.start_addr = CY_SRAM_BASE, .length = CY_SRAM_SIZE},
  };

  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

int memfault_platform_boot(void) {
  memfault_build_info_dump();
  memfault_device_info_dump();

  memfault_freertos_port_boot();
  memfault_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[MEMFAULT_EVENT_STORAGE_RAM_SIZE];
  static uint8_t s_log_storage[MEMFAULT_LOG_STORAGE_RAM_SIZE];
  const sMemfaultEventStorageImpl *evt_storage =
      memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  memfault_trace_event_boot(evt_storage);

  memfault_reboot_tracking_collect_reset_info(evt_storage);

  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  memfault_log_boot(s_log_storage, sizeof(s_log_storage));

  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
