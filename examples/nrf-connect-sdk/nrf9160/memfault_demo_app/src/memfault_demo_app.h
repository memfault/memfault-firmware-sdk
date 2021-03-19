#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

//! Start watchdog subsystem for demo application
void memfault_demo_app_watchdog_boot(void);

//! Reset watchdog timeout for application
void memfault_demo_app_watchdog_feed(void);
