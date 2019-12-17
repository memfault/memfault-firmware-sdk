#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//! @brief
//! Demonstration CLI used by main

#include "memfault/core/compiler.h"

void cli_init(void);
MEMFAULT_NORETURN void cli_forever(void);
