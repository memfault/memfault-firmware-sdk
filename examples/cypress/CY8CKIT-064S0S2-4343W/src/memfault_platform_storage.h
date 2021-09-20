//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Persistent storage using chip internal flash.
//!

#pragma once

#include <cyhal_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flash block object */
extern cyhal_flash_t flash_obj;
extern cyhal_flash_info_t flash_obj_info;

/* Coredumps section addresses from the linker script. */
extern uint8_t __MfltCoredumpsStart;
extern uint8_t __MfltCoredumpsEnd;

#ifdef __cplusplus
}
#endif
