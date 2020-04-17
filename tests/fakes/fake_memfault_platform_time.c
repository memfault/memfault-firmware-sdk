//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "fake_memfault_platform_time.h"

#include <stdbool.h>

typedef struct {
  bool enabled;
  sMemfaultCurrentTime time;
} sMemfaultFakeTimerState;

static sMemfaultFakeTimerState s_fake_timer_state;

void fake_memfault_platform_time_enable(bool enable) {
  s_fake_timer_state.enabled = enable;
}

void fake_memfault_platform_time_set(const sMemfaultCurrentTime *time) {
  s_fake_timer_state.time = *time;
}

//
// fake implementation
//

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  if (!s_fake_timer_state.enabled) {
    return false;
  }

  *time = s_fake_timer_state.time;

  return true;
}
