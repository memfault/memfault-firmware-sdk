#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! A lightweight set of log utilities which can be wrapped around pre-existing logging
//! infrastructure to capture events or errors that transpired leading up to a crash. Any logs
//! captured by the module will appear in the "Logs" view for issues in the Memfault Web UI.
//!
//! @note These utilities are already integrated into memfault/core/debug_log.h module. If your
//! project does not have a logging subsystem, see the notes in that header about how to leverage
//! the debug_log.h module for that!
//!
//! @note The thread-safety of the module depends on memfault_lock/unlock() API. If calls can be
//! made from multiple tasks, these APIs must be implemented.

#include <stdbool.h>
#include <stddef.h>

#include "memfault/core/compiler.h"
#include "memfault/core/platform/debug_log.h" // For eMemfaultPlatformLogLevel

#ifdef __cplusplus
extern "C" {
#endif

//! Must be called on boot to initialize the Memfault logging module
//!
//! @param buffer The ram buffer to save logs into
//! @param buf_len The length of the buffer. There's no length restriction but
//!  the more space that is available, the longer the trail of breadcrumbs that will
//!  be available upon crash
//!
//! @note Until this function is called, all other calls to the module will be no-ops
//! @return true if call was successful and false if the parameters were bad or
//!  memfault_log_boot has already been called
bool memfault_log_boot(void *buffer, size_t buf_len);

//! Change the minimum level log saved to the circular buffer
//!
//! By default, any logs >= kMemfaultPlatformLogLevel_Info can be saved
void memfault_log_set_min_save_level(eMemfaultPlatformLogLevel min_log_level);

//! Macro which can be called from a platforms pre-existing logging macro
//!
//! For example, if your platform already has something like this
//!
//! #define YOUR_PLATFORM_LOG_ERROR(...)
//!   my_platform_log_error(__VA_ARGS__)
//!
//! the error data could be automatically recorded by making the
//! following modification
//!
//! #define YOUR_PLATFORM_LOG_ERROR(...)
//!  do {
//!    MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Error, __VA_ARGS__);
//!    your_platform_log_error(__VA_ARGS__)
//! } while (0)
#define MEMFAULT_LOG_SAVE(_level, ...) memfault_log_save(_level, __VA_ARGS__)

//! Function which can be called to save a log after it has been formatted
//!
//! Typically a user should be able to use the MEMFAULT_LOG_SAVE macro but if your platform does
//! not have logging macros and you are just using newlib or dlib & printf, you could make
//! the following changes to the _write dependency function:
//!
//! int _write(int fd, char *ptr, int len) {
//!   // ... other code such as printing to console ...
//!   eMemfaultPlatformLogLevel level =
//!       (fd == 2) ? kMemfaultPlatformLogLevel_Error : kMemfaultPlatformLogLevel_Info;
//!   memfault_log_save_preformatted(level, ptr, len);
//!   // ...
//! }
void memfault_log_save_preformatted(eMemfaultPlatformLogLevel level, const char *log,
                                    size_t log_len);

//! Maximum length a log record can occupy
#define MEMFAULT_LOG_MAX_LINE_SAVE_LEN 128

//! Formats the provided string and saves it to backing storage
//!
//! @note: Should only be called via MEMFAULT_LOG_SAVE macro
MEMFAULT_PRINTF_LIKE_FUNC(2, 3)
void memfault_log_save(eMemfaultPlatformLogLevel level, const char *fmt, ...);

//! Reset the state of log tracking
//!
//! @note Internal function only intended for use with unit tests
void memfault_log_reset(void);

#ifdef __cplusplus
}
#endif
