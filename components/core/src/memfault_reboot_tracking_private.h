#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Internal utilities used for tracking reboot reasons

#include <inttypes.h>
#include <stdbool.h>

#include "memfault/core/reboot_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef MEMFAULT_PACKED_STRUCT MfltRebootInfo {
  //! A cheap way to check if the data within the struct is valid
  uint32_t magic;
  //! Version of the struct. If a new field is added it should be appended right before rsvd. This
  //! way we can remain backwards compatible but know what fields are valid.
  uint8_t version;
  //! The number of times the system has reset due to an error
  //! without any crash data being read out via the Memfault packetizer
  uint8_t crash_count;
  uint8_t rsvd1[1];
  uint8_t coredump_saved;
  uint32_t last_reboot_reason;  // eMemfaultRebootReason or MEMFAULT_REBOOT_REASON_NOT_SET
  uint32_t pc;
  uint32_t lr;
  //! Most MCUs have a register which reveals why a device rebooted.
  //!
  //! This can be particularly useful for debugging reasons for unexpected reboots
  //! (where no coredump was saved or no user initiated reset took place). Examples
  //! of this include brown out resets (BORs) & hardware watchdog resets.
  uint32_t reset_reason_reg0;
  // Reserved for future additions
  uint32_t rsvd2[10];
}
sMfltRebootInfo;

typedef struct MfltResetReasonInfo {
  eMemfaultRebootReason reason;
  uint32_t pc;
  uint32_t lr;
  uint32_t reset_reason_reg0;
  bool coredump_saved;
} sMfltResetReasonInfo;

//! Clears any crash information which was stored
void memfault_reboot_tracking_clear_reset_info(void);

//! Clears stored reboot reason stored at bootup
void memfault_reboot_tracking_clear_reboot_reason(void);

bool memfault_reboot_tracking_read_reset_info(sMfltResetReasonInfo *info);

#ifdef __cplusplus
}
#endif
