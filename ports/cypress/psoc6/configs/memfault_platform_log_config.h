//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
// Logging depends on how your configuration does logging. See
// https://docs.memfault.com/docs/mcu/self-serve/#logging-dependency

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "memfault/core/log.h"

#define MEMFAULT_LOG_DEBUG(fmt, ...)                                        \
  do {                                                                      \
    MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Debug, fmt, ##__VA_ARGS__); \
    printf("[D] " fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define MEMFAULT_LOG_INFO(fmt, ...)                                        \
  do {                                                                     \
    MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Info, fmt, ##__VA_ARGS__); \
    printf("[I] " fmt "\n", ##__VA_ARGS__);                                \
  } while (0)

#define MEMFAULT_LOG_WARN(fmt, ...)                                           \
  do {                                                                        \
    MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Warning, fmt, ##__VA_ARGS__); \
    printf("[W] " fmt "\n", ##__VA_ARGS__);                                   \
  } while (0)

#define MEMFAULT_LOG_ERROR(fmt, ...)                                        \
  do {                                                                      \
    MEMFAULT_LOG_SAVE(kMemfaultPlatformLogLevel_Error, fmt, ##__VA_ARGS__); \
    printf("[E] " fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define MEMFAULT_LOG_RAW(fmt, ...)   \
  do {                               \
    printf(fmt "\n", ##__VA_ARGS__); \
  } while (0)

#ifdef __cplusplus
}
#endif
