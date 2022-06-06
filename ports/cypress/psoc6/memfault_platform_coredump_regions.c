//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A coredump port which scoops up all the FreeRTOS state in the system at the time of crash
//! To override and provide a custom port, simply add this file to the .cyignore for your project
//! so it is not included in the build:
//! (i.e $(SEARCH_memfault-firmware-sdk)/ports/cypress/psoc6/memfault_platform_coredump_regions.c)

#include <stddef.h>
#include <string.h>

#include "memfault/components.h"
#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/ports/freertos_coredump.h"

#include "cy_syslib.h"
#include "cy_device_headers.h"

#define MEMFAULT_COREDUMP_MAX_TASK_REGIONS ((MEMFAULT_PLATFORM_MAX_TRACKED_TASKS) * 2)

static sMfltCoredumpRegion s_coredump_regions[
    MEMFAULT_COREDUMP_MAX_TASK_REGIONS
    + 2 /* active stack(s) */
    + 1 /* _kernel variable */
    + 1 /* __memfault_capture_start */
];

//! Note: If you get a linker error because these symbols are missing, your forgot to add the Memfault ld file
//! to you LDFLAGS:
//!  LDFLAGS += -T$(SEARCH_memfault-firmware-sdk)/ports/cypress/psoc6/memfault_bss.ld
extern uint32_t __memfault_capture_bss_end;
extern uint32_t __memfault_capture_bss_start;

//! Note: This function is called early on boot before the C runtime is loaded
//! We hook into this function in order to scrub the
void Cy_OnResetUser(void) {
  const size_t memfault_region_size = (uint32_t)&__memfault_capture_bss_end -
      (uint32_t)&__memfault_capture_bss_start;
  memset((uint32_t*)&__memfault_capture_bss_start, 0x0, memfault_region_size);
}

const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  int region_idx = 0;
  const size_t active_stack_size_to_collect = 512;

  // first, capture the active stack (and ISR if applicable)
  const bool msp_was_active = (crash_info->exception_reg_state->exc_return & (1 << 2)) == 0;

  size_t stack_size_to_collect = memfault_platform_sanitize_address_range(
      crash_info->stack_address, MEMFAULT_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

  s_coredump_regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
      crash_info->stack_address, stack_size_to_collect);
  region_idx++;

  if (msp_was_active) {
    // System crashed in an ISR but the running task state is on PSP so grab that too
    void *psp = (void *)(uintptr_t)__get_PSP();

    // Collect a little bit more stack for the PSP since there is an
    // exception frame that will have been stacked on it as well
    const uint32_t extra_stack_bytes = 128;
    stack_size_to_collect = memfault_platform_sanitize_address_range(
        psp, active_stack_size_to_collect + extra_stack_bytes);
    s_coredump_regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
        psp, stack_size_to_collect);
    region_idx++;
  }

  // Scoop up memory regions necessary to perform unwinds of the FreeRTOS tasks
  const size_t memfault_region_size = (uint32_t)&__memfault_capture_bss_end -
      (uint32_t)&__memfault_capture_bss_start;

  s_coredump_regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
     &__memfault_capture_bss_start, memfault_region_size);
  region_idx++;

  region_idx += memfault_freertos_get_task_regions(&s_coredump_regions[region_idx],
      MEMFAULT_ARRAY_SIZE(s_coredump_regions) - region_idx);

  *num_regions = region_idx;
  return &s_coredump_regions[0];
}
