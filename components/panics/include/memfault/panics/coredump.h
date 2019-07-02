//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Infra for collecting backtraces which can be parsed by memfault!

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Invoked by assert handler to capture coredump.
//!
//! @param regs Capture of all the registers at the time of capture
//! @param size The size of register collection
//! @param trace_reason The reason for collecting the coredump
void memfault_coredump_save(void *regs, size_t size, uint32_t trace_reason);

//! Queries whether a valid coredump is present in the coredump storage.
//!
//! @param total_size_out Upon returning from the function, the size of the coredump
//! in bytes has been written to the variable.
//! @return true when a valid coredump is present in the storage.
bool memfault_coredump_has_valid_coredump(size_t *total_size_out);

#ifdef __cplusplus
}
#endif
