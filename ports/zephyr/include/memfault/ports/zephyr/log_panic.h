#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief Memfault Zephyr log backend APIs

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MEMFAULT_FAULT_HANDLER_LOG_PANIC)
  #define MEMFAULT_LOG_PANIC() LOG_PANIC()
#else
  #define MEMFAULT_LOG_PANIC()
#endif

#ifdef __cplusplus
}
#endif
