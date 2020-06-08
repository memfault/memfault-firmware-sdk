//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "sdk_config.h"

#include "app_error.h"
#include "app_util.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "nrf_stack_guard.h"

#include "memfault/core/platform/reboot_tracking.h"
#include "memfault/core/reboot_tracking.h"

static uint32_t prv_reset_reason_get(void) {
  if (nrf_sdh_is_enabled()) {
    uint32_t reset_reas = 0;
    sd_power_reset_reason_get(&reset_reas);
    return reset_reas;
  }

  return NRF_POWER->RESETREAS;
}

static void prv_reset_reason_clear(uint32_t reset_reas_clear_mask) {
  if (nrf_sdh_is_enabled()) {
    sd_power_reset_reason_clr(reset_reas_clear_mask);
  } else {
    NRF_POWER->RESETREAS |= reset_reas_clear_mask;
  }
}

void memfault_platform_reboot_tracking_boot(void) {
  // NOTE: Since the NRF52 SDK .ld files are based on the CMSIS ARM Cortex-M linker scripts, we use
  // the bottom of the main stack to hold the 64 byte reboot reason.
  //
  // For a detailed explanation about reboot reason storage options check out the guide at:
  //    https://mflt.io/reboot-reason-storage

  uint32_t reboot_tracking_start_addr = (uint32_t)STACK_BASE;
#if NRF_STACK_GUARD_ENABLED
  reboot_tracking_start_addr += STACK_GUARD_SIZE;
#endif

  const uint32_t reset_reas_reg_val = prv_reset_reason_get();
  const sResetBootupInfo reset_reason = {
    .reset_reason_reg = reset_reas_reg_val,
  };
  memfault_reboot_tracking_boot((void *)reboot_tracking_start_addr, &reset_reason);

  // RESETREAS is part of the always on power domain so it's sticky until a full reset occurs
  // Therefore, we clear the bits which were set so that don't get logged in future reboots as well
  prv_reset_reason_clear(reset_reas_reg_val);
}
