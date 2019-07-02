#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs the platform must implement for using the SDK. These routines are needed by all
//! subcomponents of the library one can include

#include <stddef.h>
#include <stdint.h>

#include "memfault/core/compiler.h"
#include "memfault/core/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Initialize your platform code
//! @return @ref MemfaultReturnCode_Ok if initialization completed
//!         @ref MemfaultReturnCode_Error if not
MemfaultReturnCode memfault_platform_boot(void);

//! Invoked after memfault fault handling has run.
//!
//! The platform should do any final cleanup and reboot the system
MEMFAULT_NORETURN void memfault_platform_reboot(void);

//! Invoked after faults occur so the user can debug locally if a debugger is attached
void memfault_platform_halt_if_debugging(void);

//! @return elapsed time since the device was last restarted (in milliseconds)
uint64_t memfault_platform_get_time_since_boot_ms(void);

#ifdef __cplusplus
}
#endif
