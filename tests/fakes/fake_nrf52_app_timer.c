//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A fake implementation of the nrf52 app_timer.h. Simulates a timer which expires immediately and
//! invokes the callback scheduled

#include "app_timer.h"

#include <inttypes.h>

uint32_t app_timer_start(app_timer_id_t timer_id, uint32_t timeout_ticks, void *p_context) {
  timer_id->handler(p_context);
  return 0;
}

uint32_t app_timer_create(app_timer_id_t const *p_timer_id,
                          app_timer_mode_t mode,
                          app_timer_timeout_handler_t timeout_handler) {
  (*p_timer_id)->handler = timeout_handler;
  return 0;
}
