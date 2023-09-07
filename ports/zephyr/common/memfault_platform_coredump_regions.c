//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! The default regions to collect on Zephyr when a crash takes place.
//! Function is defined as weak so an end user can override it.

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(kernel.h)
#include MEMFAULT_ZEPHYR_INCLUDE(kernel_structs.h)

#include "memfault/panics/arch/arm/cortex_m.h"
#include "memfault/panics/platform/coredump.h"
#include "memfault/ports/zephyr/version.h"

#if MEMFAULT_ZEPHYR_VERSION_GT(3, 4)
  #include <cmsis_core.h>
#elif MEMFAULT_ZEPHYR_VERSION_GT(2, 1)
  #include MEMFAULT_ZEPHYR_INCLUDE(arch/arm/aarch32/cortex_m/cmsis.h)
#else
  #include MEMFAULT_ZEPHYR_INCLUDE(arch/arm/cortex_m/cmsis.h)
#endif

#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/ports/zephyr/coredump.h"
// clang-format on

size_t memfault_zephyr_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                            sMfltCoredumpRegion *regions, size_t num_regions) {
  // Check that regions is valid and has enough space to store all required regions
  if (regions == NULL || num_regions < MEMFAULT_ZEPHYR_COREDUMP_REGIONS) {
    return 0;
  }

  size_t region_idx = 0;

#if CONFIG_MEMFAULT_COREDUMP_COLLECT_STACK_REGIONS
  const bool msp_was_active = (crash_info->exception_reg_state->exc_return & (1 << 2)) == 0;

  size_t stack_size_to_collect = memfault_platform_sanitize_address_range(
    crash_info->stack_address, CONFIG_MEMFAULT_COREDUMP_STACK_SIZE_TO_COLLECT);

  regions[region_idx] =
    MEMFAULT_COREDUMP_MEMORY_REGION_INIT(crash_info->stack_address, stack_size_to_collect);
  region_idx++;

  if (msp_was_active) {
    // System crashed in an ISR but the running task state is on PSP so grab that too
    void *psp = (void *)(uintptr_t)__get_PSP();

    // Collect a little bit more stack for the PSP since there is an
    // exception frame that will have been stacked on it as well
    const uint32_t extra_stack_bytes = 128;
    stack_size_to_collect = memfault_platform_sanitize_address_range(
      psp, CONFIG_MEMFAULT_COREDUMP_STACK_SIZE_TO_COLLECT + extra_stack_bytes);
    regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(psp, stack_size_to_collect);
    region_idx++;
  }
#endif

#if CONFIG_MEMFAULT_COREDUMP_COLLECT_KERNEL_REGION
  regions[region_idx] = MEMFAULT_COREDUMP_MEMORY_REGION_INIT(&_kernel, sizeof(_kernel));
  region_idx++;
#endif

#if CONFIG_MEMFAULT_COREDUMP_COLLECT_TASKS_REGIONS
  region_idx += memfault_zephyr_get_task_regions(&regions[region_idx], num_regions - region_idx);
#endif

  //
  // Now that we have captured all the task state, we will
  // fill whatever space remains in coredump storage with the
  // data and bss we can collect!
  //

#if CONFIG_MEMFAULT_COREDUMP_COLLECT_DATA_REGIONS
  region_idx += memfault_zephyr_get_data_regions(&regions[region_idx], num_regions - region_idx);
#endif

#if CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS
  region_idx += memfault_zephyr_get_bss_regions(&regions[region_idx], num_regions - region_idx);
#endif

  return region_idx;
}

MEMFAULT_WEAK
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  static sMfltCoredumpRegion s_coredump_regions[MEMFAULT_ZEPHYR_COREDUMP_REGIONS];

  *num_regions = memfault_zephyr_coredump_get_regions(crash_info, s_coredump_regions,
                                                      MEMFAULT_ARRAY_SIZE(s_coredump_regions));
  return &s_coredump_regions[0];
}
