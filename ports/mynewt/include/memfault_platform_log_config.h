#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#ifdef __cplusplus
extern "C" {
#endif

#include "console/console.h"

#define _MEMFAULT_LOG_IMPL(fmt, ...)            \
  console_printf(fmt "\n", ## __VA_ARGS__);

#define MEMFAULT_LOG_DEBUG(fmt, ...) _MEMFAULT_LOG_IMPL("<dbg> " fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_INFO(fmt, ...)  _MEMFAULT_LOG_IMPL("<inf> " fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_WARN(fmt, ...)  _MEMFAULT_LOG_IMPL("<wrn> " fmt, ## __VA_ARGS__)
#define MEMFAULT_LOG_ERROR(fmt, ...) _MEMFAULT_LOG_IMPL("<err> " fmt, ## __VA_ARGS__)

#define MEMFAULT_LOG_RAW(fmt, ...) console_printf(fmt "\n", ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif
