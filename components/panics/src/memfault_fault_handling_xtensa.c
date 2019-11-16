//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handling for Xtensa based devices

#include "memfault/panics/reboot_tracking.h"
#include "memfault_reboot_tracking_private.h"

#include <stdlib.h>

#ifndef ESP_PLATFORM
#  error "The current assert handler assumes the esp-idf is in use!"
#endif

static eMfltResetReason s_crash_reason = kMfltRebootReason_Unknown;

void memfault_fault_handling_assert(void *pc, void *lr, uint32_t extra) {
  // Note: At the moment we assume the esp-idf is in use when Xtensa is being used. The SDK
  // implements abort which will invoke the esp-idf coredump handler as well as a chip reboot.
  abort();
}
