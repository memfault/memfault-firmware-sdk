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

#define MEMFAULT_LOG_DEBUG(fmt, ...) printf( "[D] " fmt "\n", ## __VA_ARGS__)
#define MEMFAULT_LOG_INFO(fmt, ...)  printf( "[I] " fmt "\n", ## __VA_ARGS__)
#define MEMFAULT_LOG_WARN(fmt, ...)  printf( "[W] " fmt "\n", ## __VA_ARGS__)
#define MEMFAULT_LOG_ERROR(fmt, ...) printf( "[E] " fmt "\n", ## __VA_ARGS__)
#define MEMFAULT_LOG_RAW(fmt, ...) printf( fmt "\n", ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif
