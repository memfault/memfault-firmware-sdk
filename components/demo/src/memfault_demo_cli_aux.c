//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Some variables that are used in demo applications to force certain crashes
//! We put them in their own compilation unit so the compiler can't figure out

#include <stdint.h>

#include "memfault/core/compiler.h"

// Jump through some hoops to trick the compiler into doing an unaligned 64 bit access
static uint8_t s_test_buffer[16] MEMFAULT_ALIGNED(4);
void *g_memfault_unaligned_buffer = &s_test_buffer[1];
// Also jump through some more hoops to trick the compiler into executing a bad function
void (*g_bad_func_call)(void) = (void *)0xbadcafe;
