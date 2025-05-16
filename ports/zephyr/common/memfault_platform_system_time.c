//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! Memfault system time provider for Zephyr.

#include "memfault/core/compiler.h"
#include "memfault/core/platform/system_time.h"

// clang-format off
#include MEMFAULT_ZEPHYR_INCLUDE(logging/log.h)
// clang-format on

// Disable logging in this module. memfault_platform_time_get_current() can be
// called from the Memfault logging subsystem, and if that function then logs,
// it can infinitely recurse.
LOG_MODULE_REGISTER(mflt_systime, LOG_LEVEL_NONE);

// This implementation uses the Nordic Date-Time library:
// https://docs.nordicsemi.com/bundle/ncs-2.5.0/page/nrf/libraries/others/date_time.html
#if defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_DATETIME)
  #include <date_time.h>

// event handler for when date/time is updated
static bool date_time_is_ready = false;
void memfault_zephyr_date_time_evt_handler(const struct date_time_evt *evt) {
  LOG_DBG("Date time event handler: %d", evt->type);

  if (evt->type == DATE_TIME_NOT_OBTAINED) {
    LOG_WRN("failed to obtain date-time");
    if (date_time_is_ready) {
      LOG_ERR("date-time lost");
      date_time_is_ready = false;
    }
  } else {
    date_time_is_ready = true;

    int64_t unix_time_ms = 0;
    int err = date_time_now(&unix_time_ms);
    LOG_DBG("date-time obtained: err=%d, %u.%us", err, (uint32_t)(unix_time_ms / 1000),
            (uint32_t)(unix_time_ms % 1000));
  }
}

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  if (date_time_is_ready) {
    int64_t unix_time_ms;
    int err = date_time_now(&unix_time_ms);

    if (!err) {
      *time = (sMemfaultCurrentTime){
        .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
        .info = {
          .unix_timestamp_secs = unix_time_ms / 1000,
        },
      };
      return true;
    }
  }

  // otherwise, unable to retrieve time
  return false;
}
#endif  // defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_DATETIME)

#if defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_RTC)
// Zephyr RTC subsystem based system time
  #include <time.h>
  #include MEMFAULT_ZEPHYR_INCLUDE(drivers/rtc.h)

  // Use RTC node if it exists, otherwise use an alias
  #if DT_NODE_EXISTS(DT_NODELABEL(rtc))
    #define MFLT_RTC_NODE DT_NODELABEL(rtc)
  #else
    #define MFLT_RTC_NODE DT_ALIAS(rtc)
  #endif

  #if !DT_NODE_HAS_STATUS(MFLT_RTC_NODE, okay)
    #error "Error, requires rtc device tree entry"
  #endif

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  struct rtc_time rtctime;
  const struct device *rtc = DEVICE_DT_GET(MFLT_RTC_NODE);
  rtc_get_time(rtc, &rtctime);

  // Debug: print time fields
  LOG_DBG("Time: %u-%u-%u %u:%u:%u", rtctime.tm_year + 1900, rtctime.tm_mon + 1, rtctime.tm_mday,
          rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);

  // If pre-2023, something is wrong
  if ((rtctime.tm_year < 123) || (rtctime.tm_year > 200)) {
    LOG_WRN("Time doesn't make sense: year %u", rtctime.tm_year + 1900);
    return false;
  }

  struct tm *tm_time = rtc_time_to_tm(&rtctime);
  time_t time_now = mktime(tm_time);

  if (time_now == (time_t)-1) {
    LOG_ERR("Error converting time");
    return false;
  }

  LOG_DBG("Setting event time %llu", (uint64_t)time_now);
  // load the timestamp and return true for a valid timestamp
  *time = (sMemfaultCurrentTime){
    .type = kMemfaultCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = (uint64_t)time_now,
    },
  };
  return true;
}
#endif  // defined(CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_RTC)
