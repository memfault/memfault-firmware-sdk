//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief A RAM-backed implementation used for tracking state across system reboots.
//! Assumptions:
//!  - RAM state survives across resets (this is generally true as long as power is stable)
//!    Otherwise, this implementation will behave as a no-op
//!  - The memory which needs to persist in RAM must not be initialized by any of the firmwares
//!    upon reboot & the memory must be placed in the same region for the firmwares running on
//!    the system (i.e bootloader & main image). In GCC LD this can be pretty easily
//!    achieved by defining the region in MEMORY and placing the tracking struct in SECTIONS:
//!       NOINIT (rw) :  ORIGIN = <addr>, LENGTH = 0x100
//!      .noinit (NOLOAD): { KEEP(*(*.mflt_reboot_info)) } > NOINIT

#include "memfault/panics/reboot_tracking.h"
#include "memfault_reboot_tracking_private.h"

#include <stddef.h>
#include <inttypes.h>

#include "memfault/core/compiler.h"

#define MEMFAULT_REBOOT_INFO_MAGIC 0x21544252

#define MEMFAULT_REBOOT_INFO_VERSION 1

typedef struct MEMFAULT_PACKED MfltRebootInfo {
  //! A cheap way to check if the data within the struct is valid
  uint32_t magic;
  //! Version of the struct. If a new field is added it should be appended right before rsvd. This
  //! way we can remain backwards compatible but know what fields are valid.
  uint8_t version;
  //! These fields are used by the bootloader & normal application
  uint8_t crash_count;
  uint8_t app_launch_attempts;
  uint8_t padding;
  //! Only the normal application uses these fields
  uint32_t last_reboot_reason; // eMfltRebootReason
  uint32_t pc;
  uint32_t lr;
  uint8_t rsvd[236];
} sMfltRebootInfo;

MEMFAULT_STATIC_ASSERT(sizeof(sMfltRebootInfo) == MEMFAULT_REBOOT_TRACKING_REGION_SIZE,
                   "struct doesn't match expected size");

static sMfltRebootInfo *s_mflt_reboot_info;

static void prv_check_or_init_struct(void) {
  if (s_mflt_reboot_info->magic == MEMFAULT_REBOOT_INFO_MAGIC) {
    return;
  }

  // structure doesn't match what we expect, reset it
  *s_mflt_reboot_info = (sMfltRebootInfo) {
    .magic = MEMFAULT_REBOOT_INFO_MAGIC,
    .version = MEMFAULT_REBOOT_INFO_VERSION,
  };
}

MemfaultReturnCode memfault_reboot_tracking_boot(void *start_addr) {
  if (start_addr == NULL) {
    return MemfaultReturnCode_InvalidInput;
  }

  s_mflt_reboot_info = start_addr;
  prv_check_or_init_struct();
  return MemfaultReturnCode_Ok;
}

void memfault_reboot_tracking_mark_app_launch_attempted(void) {
  if (s_mflt_reboot_info == NULL) {
    return;
  }

  prv_check_or_init_struct();
  s_mflt_reboot_info->app_launch_attempts++;
}

void memfault_reboot_tracking_mark_crash(const sMfltCrashInfo *info) {
  if ((s_mflt_reboot_info == NULL) || (info == NULL)) {
    return;
  }

  prv_check_or_init_struct();
  s_mflt_reboot_info->crash_count++;

  // Store the first crash observed ... where bad things started!
  if (s_mflt_reboot_info->last_reboot_reason == 0) {
    s_mflt_reboot_info->last_reboot_reason = info->reason;
    s_mflt_reboot_info->pc = info->pc;
    s_mflt_reboot_info->lr = info->lr;
  }
}

void memfault_reboot_tracking_mark_system_stable(void) {
  if (s_mflt_reboot_info == NULL) {
    return;
  }

  prv_check_or_init_struct();
  s_mflt_reboot_info->crash_count = 0;
}

void memfault_reboot_tracking_mark_system_started(void) {
  if (s_mflt_reboot_info == NULL) {
    return;
  }

  prv_check_or_init_struct();
  s_mflt_reboot_info->app_launch_attempts = 0;
}

bool memfault_reboot_tracking_is_firmware_unstable(void) {
  if (s_mflt_reboot_info == NULL) {
    return false;
  }

  prv_check_or_init_struct();

  const size_t num_resets_for_unstable = 3;
  return ((s_mflt_reboot_info->crash_count >= num_resets_for_unstable) ||
          (s_mflt_reboot_info->app_launch_attempts >= num_resets_for_unstable));
}

bool memfault_reboot_tracking_get_crash_info(sMfltCrashInfo *info) {
  if ((s_mflt_reboot_info == NULL) || (info == NULL)) {
    return false;
  }

  prv_check_or_init_struct();
  if (s_mflt_reboot_info->last_reboot_reason == 0) {
    return false; // no reset crashes!
  }
  *info = (sMfltCrashInfo) {
    .reason = s_mflt_reboot_info->last_reboot_reason,
    .pc = s_mflt_reboot_info->pc,
    .lr = s_mflt_reboot_info->lr,
  };

  return true;
}

void memfault_reboot_tracking_clear_crash_info(void) {
  if (s_mflt_reboot_info == NULL) {
    return;
  }

  s_mflt_reboot_info->last_reboot_reason = 0;
  s_mflt_reboot_info->pc = 0;
  s_mflt_reboot_info->lr = 0;
}
