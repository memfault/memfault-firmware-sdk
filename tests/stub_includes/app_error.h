#pragma once

//! @file
//!
//! Copyright (c) 2019-Present Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A minimal implementation of app_error.h dependencies needed for unit testing.  This way the
//! entire nrf5 sdk is not needed to run a unit test

#include <assert.h>
#include <inttypes.h>

typedef uint32_t ret_code_t;

#define APP_ERROR_CHECK(x) assert((x) == 0)
