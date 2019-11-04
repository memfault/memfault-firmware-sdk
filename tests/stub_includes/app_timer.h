#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A minimal stub of the nrf52 app_timer.h header. Ideally no files would depend on it at all
//! (because we should be sufficiently wrapping the timer impl).

#include <inttypes.h>

typedef enum {
  APP_TIMER_MODE_SINGLE_SHOT,
  APP_TIMER_MODE_REPEATED,
} app_timer_mode_t;

typedef void (*app_timer_timeout_handler_t)(void *p_context);

typedef struct {
  app_timer_timeout_handler_t handler;
} app_timer_t;

typedef app_timer_t *app_timer_id_t;

uint32_t app_timer_start(app_timer_id_t timer_id, uint32_t timeout_ticks, void *p_context);

uint32_t app_timer_create(app_timer_id_t const *p_timer_id,
                          app_timer_mode_t mode,
                          app_timer_timeout_handler_t timeout_handler);

#define APP_TIMER_DEF(timer_id) \
  static app_timer_t s_timer_struct##timer_id;  \
  static const app_timer_id_t timer_id = &s_timer_struct##timer_id

#define APP_TIMER_TICKS(x) x
