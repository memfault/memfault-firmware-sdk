#pragma once

//! @file memfault_platform_config.h
//!
//! Copyright 2022 Memfault, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! @brief
//! Platform overrides for the default configuration settings in the memfault-firmware-sdk.
//! Default configuration settings can be found in "memfault/config.h"

/*****************************************************************************
 * Included Files
 *****************************************************************************/

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// For example, decide if you want to use the Gnu Build ID.
// #define MEMFAULT_USE_GNU_BUILD_ID 1

#define MEMFAULT_PLATFORM_HAS_LOG_CONFIG 1

#define MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS 600

#define MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH 1

#ifdef __cplusplus
}
#endif


/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/