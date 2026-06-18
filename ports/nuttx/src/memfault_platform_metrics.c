//! @file memfault_platform_metrics
//!
//! Copyright 2022 Memfault, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! Glue layer between the Memfault SDK and the Nuttx platform


/*****************************************************************************
 * Included Files
 *****************************************************************************/

#include "memfault/components.h"
#include "memfault/ports/reboot_reason.h"

#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include <nuttx/clock.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/**
 * @brief Static variable used to store the memfault passed callback
 * 
 */
static MemfaultPlatformTimerCallback *metric_timer_handler;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memfault_platform_metric_timer_dispatch
 ****************************************************************************/

/**
 * @brief Memfault callback function dispatcher
 * 
 * @param handler 
 */

void memfault_platform_metric_timer_dispatch(MemfaultPlatformTimerCallback handler) {
  if (handler == NULL) {
    return;
  }
  handler();
}

/****************************************************************************
 * Name: platform_metrics_timer_handler
 ****************************************************************************/

/**
 * @brief  Timer handler
 * 
 * @param signo 
 */

static void platform_metrics_timer_handler(int signo) {

  // Check signo
  if (signo == SIGALRM) 
  {
    memfault_reboot_tracking_reset_crash_count();
    memfault_platform_metric_timer_dispatch(metric_timer_handler);
  }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: memfault_platform_metrics_timer_boot
 ****************************************************************************/

/**
 * @brief Creates a timer that executes each period_sec seconds and calls 
 * the callback function.
 * 
 * @param period_sec Period in seconds for each execution of the timer
 * @param callback Called function on timer expiration.
 * @return true 
 * @return false 
 */

bool memfault_platform_metrics_timer_boot(uint32_t period_sec,
                                          MemfaultPlatformTimerCallback callback) {
  struct sigaction act;
  struct sigaction oact;
  struct sigevent notify;
  struct itimerspec timer;
  timer_t timerid;
  int status;

  metric_timer_handler = (MemfaultPlatformTimerCallback *)callback;

  // Set timer timeout action
  act.sa_handler = &platform_metrics_timer_handler;
  act.sa_flags = SA_SIGINFO;
  (void)sigfillset(&act.sa_mask);
  (void)sigdelset(&act.sa_mask, SIGALRM);
  status = sigaction(SIGALRM, &act, &oact);
  if (status != OK) 
    {
      MEMFAULT_LOG_ERROR("Memfault metrics timer sigaction failed, status=%d\n", status);
      return ERROR;
    }

  // Create the POSIX timer
  notify.sigev_notify = SIGEV_SIGNAL;
  notify.sigev_signo = SIGALRM;
  notify.sigev_value.sival_int = 0;

#ifdef CONFIG_SIG_EVTHREAD

  notify.sigev_notify_function = NULL;
  notify.sigev_notify_attributes = NULL;

#endif

  status = timer_create(CLOCK_MONOTONIC, &notify, &timerid);
  if (status != OK) {
    MEMFAULT_LOG_ERROR("Memfault timer creation failed, errno=%d\n", errno);
    return ERROR;
  }

  // Start the POSIX timer
  timer.it_value.tv_sec = (10);
  timer.it_value.tv_nsec = (0);
  timer.it_interval.tv_sec = (period_sec);
  timer.it_interval.tv_nsec = (0);
  status = timer_settime(timerid, 0, &timer, NULL);
  if (status != OK) 
  {
    MEMFAULT_LOG_ERROR("Memfault timer start failed, errno=%d\n", errno);
    return ERROR;
  }

  return true;  
}
