#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Task watchdog APIs intended for use within the memfault-firmware-sdk

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memfault/config.h"

// Keep this after config.h
#include "memfault/core/task_watchdog.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MemfaultTaskWatchdogChannelState {
  kMemfaultTaskWatchdogChannelState_Stopped = 0,
  kMemfaultTaskWatchdogChannelState_Started,
  kMemfaultTaskWatchdogChannelState_Expired,
};
struct MemfaultTaskWatchdogChannel {
  // Using a 32-bit value to track fed_time. This is enough for 49 days, which
  // should be much longer than any watchdog timeout.
  uint32_t fed_time_ms;
  enum MemfaultTaskWatchdogChannelState state;
};
typedef struct MemfaultTaskWatchdogInfo {
  struct MemfaultTaskWatchdogChannel channels[kMemfaultTaskWatchdogChannel_NumChannels];
} sMemfaultTaskWatchdogInfo;

//! Bookkeeping for the task watchdog channels. If the structure needs to change
//! in the future, rename it to keep analyzer compatibility (eg "_v2")
extern sMemfaultTaskWatchdogInfo g_memfault_task_channel_info;

#ifdef __cplusplus
}
#endif
