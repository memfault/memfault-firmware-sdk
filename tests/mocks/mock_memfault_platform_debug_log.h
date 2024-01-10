//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

extern "C" {
#include <string.h>

#include "memfault/core/platform/debug_log.h"
}

void memfault_platform_log_set_mock(eMemfaultPlatformLogLevel level, const char *const lines[],
                                    size_t num_lines);
