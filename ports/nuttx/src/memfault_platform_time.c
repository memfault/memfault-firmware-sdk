//! @file memfault_platform_time.c
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
 * Name: memfault_platform_time_get_current
 ****************************************************************************/

/**
 * @brief Get time since epoch.
 * 
 * @param time_val 
 * @return true 
 * @return false 
 */

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time_val) {

  time_t seconds_since_epoch;

  if(time(&seconds_since_epoch) == (time_t)ERROR)
    {
      MEMFAULT_LOG_ERROR("Current calendar time is not available.");
      return false;
    }

  *time_val = (sMemfaultCurrentTime) {
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = seconds_since_epoch
    },
  };

  return true;
}

/****************************************************************************
 * Name: memfault_platform_time_get_current
 ****************************************************************************/

/**
 * @brief Get time since boot.
 * 
 * @return uint64_t 
 */

uint64_t memfault_platform_get_time_since_boot_ms(void) {
  
  uint64_t time;
  struct timespec tp;

  if(clock_gettime(CLOCK_MONOTONIC, &tp) != OK)
    {
      MEMFAULT_LOG_ERROR("Time since boot not available.");
      return time = 0;
    }

  time = (((uint64_t) tp.tv_sec) << 32) | ((uint64_t) tp.tv_nsec / NSEC_PER_MSEC);

  return time;
}

