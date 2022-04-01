#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Task watchdog API.
//!
//! This module implements a task watchdog that can be used to detect stuck RTOS
//! tasks.
//!
//! Example usage:
//!
//! // Initialize the task watchdog system on system start
//! memfault_task_watchdog_init();
//!
//! // In a task loop
//! void example_task_loop(void) {
//!   while(1) {
//!     rtos_wait_for_task_to_wake_up();
//!
//!     // Example: Use the task watchdog to monitor a block of work
//!     MEMFAULT_TASK_WATCHDOG_START(example_task_loop_channel_id);
//!     // do work that should be monitored by the watchdog
//!     MEMFAULT_TASK_WATCHDOG_STOP(example_task_loop_channel_id);
//!
//!     // Example: Use the task watchdog to monitor a repeated operation
//!     MEMFAULT_TASK_WATCHDOG_START(example_task_loop_channel_id);
//!     // feeding the watchdog is only necessary if in some long-running task
//!     // that runs the risk of expiring the timeout, but is not expected to be
//!     // stuck in between calls
//!     for (size_t i = 0; i < 100; i++) {
//!       MEMFAULT_TASK_WATCHDOG_FEED(example_task_loop_channel_id);
//!       run_slow_repeated_task();
//!     }
//!     MEMFAULT_TASK_WATCHDOG_STOP(example_task_loop_channel_id);
//!
//!     rtos_put_task_to_sleep();
//!   }
//! }
//!
//! // In either a standalone timer, or a periodic background task, call the
//! // memfault_task_watchdog_check_all function
//! void example_task_watchdog_timer_cb(void) {
//!   memfault_task_watchdog_check_all();
//! }
//!
//! // Call memfault_task_watchdog_bookkeep() in
//! // memfault_platform_coredump_save_begin()- this updates all the task
//! // channel timeouts, in the event that the task monitor wasn't able to run
//! // for some reason. Also might be appropriate to refresh other system
//! // watchdogs prior to executing the coredump save.
//! bool memfault_platform_coredump_save_begin(void) {
//!   memfault_task_watchdog_bookkeep();
//!   // may also need to refresh other watchdogs here, for example:
//!   memfault_task_watchdog_platform_refresh_callback();
//!   return true;
//! }

#include <stddef.h>

#include "memfault/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Declare the task watchdog channel IDs. These shouldn't be used directly, but
//! should be passed to the MEMFAULT_TASK_WATCHDOG_START etc. functions with the
//! name declared in the memfault_task_watchdog_config.def file.
#define MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(channel_) kMemfaultTaskWatchdogChannel_##channel_,
typedef enum {
#if MEMFAULT_TASK_WATCHDOG_ENABLE
#include "memfault_task_watchdog_config.def"
#else
  // define one channel to prevent the compiler from complaining about a zero-length array
  MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(placeholder_)
#endif
  kMemfaultTaskWatchdogChannel_NumChannels,
} eMemfaultTaskWatchdogChannel;
#undef MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE
#define MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(channel_) kMemfaultTaskWatchdogChannel_##channel_

//! Initialize (or reset) the task watchdog system. This will stop and reset all
//! internal bookkeeping for all channels.
void memfault_task_watchdog_init(void);

//! This function should be called periodically (for example, around the
//! MEMFAULT_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS period). It checks if any task
//! watchdog channel has reached the timeout interval. If so, it will trigger a
//! task watchdog assert.
void memfault_task_watchdog_check_all(void);

//! As in `memfault_task_watchdog_check_all`, but only updates the internal
//! bookkeeping, and does not trigger any callbacks or asserts.
//!
//! Intended to be called just prior to coredump capture when the system is in
//! the fault_handler.
void memfault_task_watchdog_bookkeep(void);

//! Start a task watchdog channel. After being started, a channel will now be
//! eligible for expiration. Also resets the timeout interval for the channel.
#define MEMFAULT_TASK_WATCHDOG_START(channel_id_) \
  memfault_task_watchdog_start(MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! Reset the timeout interval for a task watchdog channel.
#define MEMFAULT_TASK_WATCHDOG_FEED(channel_id_) \
  memfault_task_watchdog_feed(MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! Stop a task watchdog channel. After being stopped, a channel will no longer
//! be eligible for expiration and is reset
#define MEMFAULT_TASK_WATCHDOG_STOP(channel_id_) \
  memfault_task_watchdog_stop(MEMFAULT_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! These functions should not be used directly, but instead through the macros
//! above.
void memfault_task_watchdog_start(eMemfaultTaskWatchdogChannel channel_id);
void memfault_task_watchdog_feed(eMemfaultTaskWatchdogChannel channel_id);
void memfault_task_watchdog_stop(eMemfaultTaskWatchdogChannel channel_id);

//! Optional weakly defined function to perform any additional actions during
//! `memfault_task_watchdog_check_all` when no channels have expired
void memfault_task_watchdog_platform_refresh_callback(void);

#ifdef __cplusplus
}
#endif
