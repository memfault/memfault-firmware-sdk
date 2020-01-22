#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Infra for collecting backtraces which can be parsed by memfault!

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "memfault/panics/platform/coredump.h"
#include "memfault/panics/trace_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemfaultCoredumpSaveInfo {
  const void *regs;
  size_t regs_size;
  eMfltResetReason trace_reason;
  const sMfltCoredumpRegion *regions;
  size_t num_regions;
} sMemfaultCoredumpSaveInfo;

//! Invoked by assert handler to capture coredump.
//!
//! @note A user of the SDK shouldn't need to invoke this directly
//!
//! @param sMemfaultCoredumpSaveInfo Architecture specific information to save with the coredump
//! @return true if the coredump was saved and false if the save failed
bool memfault_coredump_save(const sMemfaultCoredumpSaveInfo *save_info);

//! Architecture specific register state
typedef struct MfltRegState sMfltRegState;

//! Handler to be invoked from fault handlers
//!
//! By default, the Memfault SDK will automatically call this function as part of
//! exception handling for the target architecture.
//!
//! @param regs The register state at the time of the fault occurred
//! @param reason The reason the fault occurred
void memfault_fault_handler(const sMfltRegState *regs, eMfltResetReason reason);

//! Computes the size required to save a coredump on the system
//!
//! @note A user of the SDK can call this on boot to assert that
//! coredump storage is large enough to capture the regions specified
//!
//! @return The space required to save the coredump or 0 on error
//! (i.e no coredump regions defined, coredump storage of 0 size)
size_t memfault_coredump_storage_compute_size_required(void);

//! Queries whether a valid coredump is present in the coredump storage.
//!
//! @param total_size_out Upon returning from the function, the size of the coredump
//! in bytes has been written to the variable.
//! @return true when a valid coredump is present in the storage.
bool memfault_coredump_has_valid_coredump(size_t *total_size_out);

#ifdef __cplusplus
}
#endif
