#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdbool.h>
#include <inttypes.h>

//! Initializes the device memfault_platform_device_info module.
bool memfault_platform_device_info_boot(void);

//! Initializes memfault_platform_coredump module.
//! @return true iff successful.
bool memfault_platform_coredump_boot(void);

//! Get the physical start and end addresses of the area that is allocated in the SPI flash
//! after calling memfault_platform_coredump_boot(). This is mainly for debugging.
//! @return true iff successful. If false is returned, it means memfault_platform_coredump_boot()
//! had not been called, or the data has been corrupted (CRC mismatch).
bool memfault_platform_get_spi_start_and_end_addr(uint32_t *flash_start, uint32_t *flash_end);
