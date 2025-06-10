#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Dependency functions which can optionally be implemented for time tracking within the Memfault
//! SDK.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kMemfaultCurrentTimeType_Unknown = 0,
  //! the number of seconds that have elapsed since the Unix epoch,
  //! (00:00:00 UTC on 1 January 1970)
  kMemfaultCurrentTimeType_UnixEpochTimeSec = 1,
} eMemfaultCurrentTimeType;

typedef struct {
  eMemfaultCurrentTimeType type;
  union {
    uint64_t unix_timestamp_secs;
  } info;
} sMemfaultCurrentTime;

//! Returns the current system time
//!
//! This dependency can (optionally) be implemented if a device keeps track of time and wants to
//! track the exact time events occurred on device. If no time is provided, the Memfault backend
//! will automatically create a timestamp based on when an event is received by the chunks endpoint.
//!
//! This function should not issue any logs, because it is called from within the Memfault logging
//! system when log timestamps are enabled.
//!
//! @note By default, a weak version of this function is implemented which always returns false (no
//! time available)
//!
//! @param return true if a time could be recovered, false otherwise
bool memfault_platform_time_get_current(sMemfaultCurrentTime *time);

#ifdef __cplusplus
}
#endif
