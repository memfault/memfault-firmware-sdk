//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Task watchdog implementation.

#include "memfault/core/task_watchdog.h"
#include "memfault/core/task_watchdog_impl.h"

//! non-module definitions

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memfault/config.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/core/platform/core.h"
#include "memfault/panics/assert.h"

#if MEMFAULT_TASK_WATCHDOG_ENABLE

sMemfaultTaskWatchdogInfo g_memfault_task_channel_info;

//! Catch if the timeout is set to an incompatible literal
static const uint32_t s_watchdog_timeout_ms = MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;

void memfault_task_watchdog_init(void) {
  g_memfault_task_channel_info = (struct MemfaultTaskWatchdogInfo){0};
}

static bool prv_memfault_task_watchdog_expired(struct MemfaultTaskWatchdogChannel channel,
                                               uint64_t current_time_ms) {
  const bool active = channel.state == kMemfaultTaskWatchdogChannelState_Started;
  // const bool expired = (channel.fed_time_ms + s_watchdog_timeout_ms) < current_time_ms;
  const bool expired = (current_time_ms - channel.fed_time_ms) > s_watchdog_timeout_ms;

  return active && expired;
}

static size_t prv_memfault_task_watchdog_do_check(void) {
  const uint32_t time_since_boot_ms = memfault_platform_get_time_since_boot_ms();

  size_t expired_channels_count = 0;
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_task_channel_info.channels); i++) {
    if (prv_memfault_task_watchdog_expired(g_memfault_task_channel_info.channels[i],
                                           time_since_boot_ms)) {
      expired_channels_count++;
    }
  }

  return expired_channels_count;
}

void memfault_task_watchdog_check_all(void) {
  size_t expired_channels_count = prv_memfault_task_watchdog_do_check();

  // if any channel reached expiration, trigger a panic
  if (expired_channels_count > 0) {
    MEMFAULT_SOFTWARE_WATCHDOG();
  } else {
    memfault_task_watchdog_platform_refresh_callback();
  }
}

void memfault_task_watchdog_bookkeep(void) {
  // only update the internal structure, don't trigger callbacks
  (void)prv_memfault_task_watchdog_do_check();
}

void memfault_task_watchdog_start(eMemfaultTaskWatchdogChannel channel_id) {
  g_memfault_task_channel_info.channels[channel_id].fed_time_ms =
    memfault_platform_get_time_since_boot_ms();
  g_memfault_task_channel_info.channels[channel_id].state =
    kMemfaultTaskWatchdogChannelState_Started;
}

void memfault_task_watchdog_feed(eMemfaultTaskWatchdogChannel channel_id) {
  g_memfault_task_channel_info.channels[channel_id].fed_time_ms =
    memfault_platform_get_time_since_boot_ms();
}

void memfault_task_watchdog_stop(eMemfaultTaskWatchdogChannel channel_id) {
  g_memfault_task_channel_info.channels[channel_id].state =
    kMemfaultTaskWatchdogChannelState_Stopped;
}

//! Callback which is called when there are no expired tasks; can be used for
//! example to reset a hardware watchdog
MEMFAULT_WEAK void memfault_task_watchdog_platform_refresh_callback(void) {}

#else  // MEMFAULT_TASK_WATCHDOG_ENABLE

void memfault_task_watchdog_bookkeep(void) {}

#endif  // MEMFAULT_TASK_WATCHDOG_ENABLE
