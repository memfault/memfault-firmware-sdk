//! @file
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
//! Logging depends on how your configuration does logging. See
//! https://docs.memfault.com/docs/mcu/self-serve/#logging-dependency


/*****************************************************************************
 * Included Files
 *****************************************************************************/

#include <syslog.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#define MEMFAULT_LOG_DEBUG(fmt, ...) syslog(LOG_DEBUG, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define MEMFAULT_LOG_INFO(fmt, ...)  syslog(LOG_INFO, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define MEMFAULT_LOG_WARN(fmt, ...)  syslog(LOG_WARNING, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#define MEMFAULT_LOG_ERROR(fmt, ...) syslog(LOG_ERR, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

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