//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <time.h>

#include "esp_log.h"
#include "memfault/components.h"

bool memfault_platform_time_get_current(sMemfaultCurrentTime *mflt_time) {
  time_t time_now = time(NULL);
  if (time_now == -1) {
    return false;
  }

  // convert to broken down time
  struct tm tm_time;
  if (gmtime_r(&time_now, &tm_time) == NULL) {
    return false;
  }

  // confirm the year is reasonable (>=2024, <=2100)
  if ((tm_time.tm_year < 124) || (tm_time.tm_year > 200)) {
    return false;
  }

  // load the timestamp and return true for a valid timestamp
  *mflt_time = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = (uint64_t)time_now,
    },
  };
  return true;
}
