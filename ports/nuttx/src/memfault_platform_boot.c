//! @file memfault_platform_boot.c
//!
//! Copyright 2022 Memfault, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! Glue layer between the Memfault SDK and the Nuttx platform

/*****************************************************************************
 * Included Files
 *****************************************************************************/

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <nuttx/clock.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memfault_platform_boot
 ****************************************************************************/

/**
 * @brief Main memfault platform function. Should be called as soon as 
 * posible at OS initialization phase.
 * 
 * @return int 
 */

int memfault_platform_boot(void) {

  memfault_build_info_dump();
  memfault_device_info_dump();
  memfault_platform_reboot_tracking_boot();

  // initialize the event storage buffer
  static uint8_t s_event_storage[1024];
  const sMemfaultEventStorageImpl *evt_storage =
    memfault_events_storage_boot(s_event_storage, sizeof(s_event_storage));

  // configure trace events to store into the buffer
  memfault_trace_event_boot(evt_storage);

  // record the current reboot reason
  memfault_reboot_tracking_collect_reset_info(evt_storage);

  // configure the metrics component to store into the buffer
  sMemfaultMetricBootInfo boot_info = {
    .unexpected_reboot_count = memfault_reboot_tracking_get_crash_count(),
  };
  memfault_metrics_boot(evt_storage, &boot_info);

  MEMFAULT_LOG_INFO("Memfault Initialized!");

  return 0;
}
