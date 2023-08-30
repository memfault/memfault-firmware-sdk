#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Headers used in Zephyr porting files that have been relocated between releases

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(sys/__assert.h)
#include MEMFAULT_ZEPHYR_INCLUDE(sys/printk.h)
// clang-format on

#ifdef __cplusplus
}
#endif
