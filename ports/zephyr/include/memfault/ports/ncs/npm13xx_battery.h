#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief A little convenience header to assist in checks which can be run at compile time for
//! backward compatibility based on NCS version.

#ifdef __cplusplus
extern "C" {
#endif

//! Call this function to inform the Memfault npm13xx metric subsystem that the
//! fuel gauge has been initialized with nrf_fuel_gauge_init() and is ready to
//! be used.
void memfault_platform_npm13xx_battery_init(void);

#ifdef __cplusplus
}
#endif
