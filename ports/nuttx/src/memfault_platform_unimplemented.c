//! @file memfault_platform_uninmplemented.c
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
 * Name: memfault_platform_halt_if_debugging
 ****************************************************************************/

/**
 * @brief memfault_platform_halt_if_debugging
 * TODO: Not yet implemented
 * 
 * @return MEMFAULT_WEAK 
 */

MEMFAULT_WEAK
void memfault_platform_halt_if_debugging(void) {
  // volatile uint32_t *dhcsr = (volatile uint32_t *)0xE000EDF0;
  // const uint32_t c_debugen_mask = 0x1;

  // if ((*dhcsr & c_debugen_mask) == 0) {
  //   // no debugger is attached so return since issuing a breakpoint instruction would trigger a
  //   // fault
  //   return;
  // }

  // // NB: A breakpoint with value 'M' (77) for easy disambiguation from other breakpoints that may
  // // be used by the system.
  // MEMFAULT_BREAKPOINT(77);
}

/****************************************************************************
 * Name: memfault_platform_sanitize_address_range
 ****************************************************************************/

/**
 * @brief memfault_platform_sanitize_address_range
 * TODO: Not yet implemented
 * 
 * @param start_addr 
 * @param desired_size 
 * @return size_t 
 */

size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  // static const struct {
  //   uint32_t start_addr;
  //   size_t length;
  // } s_mcu_mem_regions[] = {
  //   // !FIXME: Update with list of valid memory banks to collect in a coredump
  //   {.start_addr = 0x00000000, .length = 0xFFFFFFFF},
  // };

  // for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(s_mcu_mem_regions); i++) {
  //   const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
  //   const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
  //   if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
  //     return MEMFAULT_MIN(desired_size, upper_addr - (uint32_t)start_addr);
  //   }
  // }

  return 0;
}

/****************************************************************************
 * Name: memfault_platform_reboot
 ****************************************************************************/

/**
 * @brief memfault_platform_reboot
 * TODO: Not yet implemented
 */

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void memfault_platform_reboot(void) {
  // !FIXME: Perform any final system cleanup here

  // !FIXME: Reset System
  // NVIC_SystemReset()
  while (1) { } // unreachable
}