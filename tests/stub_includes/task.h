//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t portGET_RUN_TIME_COUNTER_VALUE(void);
uint32_t xTaskGetIdleRunTimeCounter(void);

#define tskKERNEL_VERSION_MAJOR 10
#define tskKERNEL_VERSION_MINOR 4
#define tskKERNEL_VERSION_BUILD 3

#ifdef __cplusplus
}
#endif
