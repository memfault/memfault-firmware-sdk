//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A stub implementation of memfault_log_save() for unit testing

#include "memfault/core/log.h"

#include "memfault/core/compiler.h"

void memfault_log_save(MEMFAULT_UNUSED eMemfaultPlatformLogLevel level,
                       MEMFAULT_UNUSED const char *fmt, ...) { }

#if MEMFAULT_COMPACT_LOG_ENABLE
void memfault_compact_log_save(MEMFAULT_UNUSED eMemfaultPlatformLogLevel level,
                               MEMFAULT_UNUSED uint32_t log_id,
                               MEMFAULT_UNUSED uint32_t compressed_fmt, ...) { }
#endif
