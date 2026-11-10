//! @file memfault_platform_info.c
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/boardctl.h>

#include <nuttx/config.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifndef CONFIG_MEMFAULT_DEVICE_SERIAL
#define CONFIG_MEMFAULT_DEVICE_SERIAL      "DEMOSERIAL" 
#endif

#ifndef CONFIG_MEMFAULT_HARDWARE_VERSION
#define CONFIG_MEMFAULT_HARDWARE_VERSION   "app-fw"
#endif

#ifndef CONFIG_MEMFAULT_SOFTWARE_TYPE
#define CONFIG_MEMFAULT_SOFTWARE_TYPE      "1.0.0"
#endif

#ifndef CONFIG_MEMFAULT_SOFTWARE_VERSION
#define CONFIG_MEMFAULT_SOFTWARE_VERSION   "dvt1"
#endif

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
 * Name: prv_int_to_ascii_hex
 ****************************************************************************/

/**
 * @brief Convert integers into ascii hex
 * 
 * @param val 
 * @return char 
 */
#ifdef CONFIG_BOARDCTL_UNIQUEID
static char prv_int_to_ascii_hex(uint8_t val) {
  return val < 10 ? (char)val + '0' : (char)(val - 10) + 'A';
}
#endif
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memfault_platform_get_device_info
 ****************************************************************************/

/**
 * @brief Get device info form Nuttx platform
 * 
 * @param info 
 */

void memfault_platform_get_device_info(sMemfaultDeviceInfo *info) {

#ifdef CONFIG_BOARDCTL_UNIQUEID

  static char s_device_serial[CONFIG_BOARDCTL_UNIQUEID_SIZE * 2 + 1];
  uint8_t uniqueid[CONFIG_BOARDCTL_UNIQUEID_SIZE];

  if (boardctl(BOARDIOC_UNIQUEID, (uintptr_t)&uniqueid) != OK)
    {
      MEMFAULT_LOG_ERROR("Board unique id failed\n");
    }
    
  for (size_t i = 0; i < CONFIG_BOARDCTL_UNIQUEID_SIZE ; i++) 
    {
      const uint8_t val = uniqueid[i];
      const int curr_idx = i * 2;
      s_device_serial[curr_idx] = prv_int_to_ascii_hex((val >> 4) & 0xf);
      s_device_serial[curr_idx + 1] = prv_int_to_ascii_hex(val & 0xf);
    }
  
  *info = (sMemfaultDeviceInfo) 
    {
      .device_serial = s_device_serial,
      .software_type = CONFIG_MEMFAULT_SOFTWARE_TYPE,
      .software_version = CONFIG_MEMFAULT_SOFTWARE_VERSION,
      .hardware_version = CONFIG_MEMFAULT_HARDWARE_VERSION,
    };
    
#else

  *info = (sMemfaultDeviceInfo) 
    {
      .device_serial = CONFIG_MEMFAULT_DEVICE_SERIAL,
      .software_type = CONFIG_MEMFAULT_SOFTWARE_TYPE,
      .software_version = CONFIG_MEMFAULT_SOFTWARE_VERSION,
      .hardware_version = CONFIG_MEMFAULT_HARDWARE_VERSION,
    };

#endif
}

