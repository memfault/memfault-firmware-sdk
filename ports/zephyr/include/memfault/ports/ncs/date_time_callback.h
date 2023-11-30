#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief interface for hooking up a date_time event callback

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_DATETIME)
  #include <date_time.h>

void memfault_zephyr_date_time_evt_handler(const struct date_time_evt *evt);

#endif

#ifdef __cplusplus
}
#endif
