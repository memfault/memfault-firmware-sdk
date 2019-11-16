#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal utilities used for tracking reboot reasons

#include <inttypes.h>
#include <stdbool.h>

#include "memfault/panics/reboot_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MfltResetReasonInfo {
  eMfltResetReason reason;
  uint32_t pc;
  uint32_t lr;
  uint32_t reset_reason_reg0;
  bool coredump_saved;
} sMfltResetReasonInfo;

//! Clears any crash information which was stored
void memfault_reboot_tracking_clear_reset_info(void);

//! Flag that there is also a coredump associated with this reset
void memfault_reboot_tracking_mark_coredump_saved(void);

bool memfault_reboot_tracking_read_reset_info(sMfltResetReasonInfo *info);

#ifdef __cplusplus
}
#endif
