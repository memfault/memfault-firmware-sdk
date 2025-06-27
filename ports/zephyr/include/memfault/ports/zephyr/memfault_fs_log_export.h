//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/core/data_packetizer_source.h"

//! Triggers export of Zephyr FS log backend data to Memfault
//!
//! @return The size in bytes of the staged file that was created for export.
size_t memfault_fs_log_trigger_export(void);

#ifdef __cplusplus
}
#endif
