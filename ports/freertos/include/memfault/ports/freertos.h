#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "FreeRTOSConfig.h"

//! By default, The Memfault FreeRTOS port determines what memory allocation scheme to
//! use based on the configSUPPORT_STATIC_ALLOCATION & configSUPPORT_DYNAMIC_ALLOCATION defines
//! within FreeRTOSConfig.h.
//!
//! Alternatively, MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION can be added to CFLAGS to
//! override this default behavior
#if !defined(MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION)

#if configSUPPORT_STATIC_ALLOCATION == 1
#define MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION 1
#elif configSUPPORT_DYNAMIC_ALLOCATION == 1
#define MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION 0
#else
#error "configSUPPORT_DYNAMIC_ALLOCATION or configSUPPORT_STATIC_ALLOCATION must be enabled in FreeRTOSConfig.h"
#endif

#endif /* MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION */


//! Should be called prior to making any Memfault SDK calls
void memfault_freertos_port_boot(void);
