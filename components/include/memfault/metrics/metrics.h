#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! The Memfault metric events API
//!
//! This APIs allows one to collect periodic events known as heartbeats for visualization in the
//! Memfault web UI. Heartbeats are a great way to inspect the overall health of devices in your
//! fleet.
//!
//! Typically, two types of information are collected:
//! 1) values taken at the end of the interval (i.e battery life, heap high water mark, stack high
//! water mark)
//! 2) changes over the hour (i.e the percent battery drop, the number of bytes sent out over
//!    bluetooth, the time the mcu was running or in stop mode)
//!
//! From the Memfault web UI, you can view all of these metrics plotted for an individual device &
//! configure alerts to fire when values are not within an expected range.
//!
//! For a step-by-step walk-through about how to integrate the Metrics component into your system,
//! check out https://mflt.io/2D8TRLX
//!
//! For more details of the overall design and how serialization compression works check out:
//!  https://mflt.io/fw-event-serialization

#include <inttypes.h>

#include "memfault/config.h"
#include "memfault/core/event_storage.h"
#include "memfault/metrics/ids_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/core/compiler.h"

//! Type of a metric value
typedef enum MemfaultMetricValueType {
  //! unsigned integer (max. 32-bits)
  kMemfaultMetricType_Unsigned = 0,
  //! signed integer (max. 32-bits)
  kMemfaultMetricType_Signed,
  //! Tracks durations (i.e the time a certain task is running, or the time a MCU is in sleep mode)
  kMemfaultMetricType_Timer,
  //! Set a string value for a metric
  kMemfaultMetricType_String,

  //! Number of valid types. Must _always_ be last
  kMemfaultMetricType_NumTypes,
} eMemfaultMetricType;

//! Defines a key/value pair used for generating Memfault events.
//!
//! This define should _only_ be used for defining events in 'memfault_metric_heartbeat_config.def'
//! i.e, the *.def file for a heartbeat which tracks battery level & temperature would look
//! something like:
//!
//! // memfault_metrics_heartbeat_config.def
//! MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(battery_level, kMemfaultMetricType_Unsigned, 0, 100)
//! MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(ambient_temperature_celcius, kMemfaultMetricType_Signed,
//! -40, 40)
//!
//! @param key_name The name of the key, without quotes. This gets surfaced in the Memfault UI, so
//! it's useful to make these names human readable. C variable naming rules apply.
//! @param value_type The eMemfaultMetricType type of the key
//! @param min_value A hint as to what the expected minimum value of the metric will be
//! @param max_value A hint as to what the expected maximum value of the metric will be
//!
//! @note min_value & max_value are used to define an expected range for a given metric.
//! This information is used in the Memfault cloud to normalize the data to a range of your
//! choosing. Metrics will still be ingested _even_ if they are outside the range defined.
//!
//! @note The definitions here are used to catch accidental usage outside of the
//! '*_heartbeat_config.def' files; 'MEMFAULT_METRICS_KEY_DEFINE_TRAP_' is a placeholder used to
//! detect
//! this.
//!
//! @note key_names must be unique
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Same as 'MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE' just with no range hints specified
#define MEMFAULT_METRICS_KEY_DEFINE(key_name, value_type) MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Declare a string metric for use in a heartbeat. 'max_length' is the maximum
//! length of the string recorded to the metric (excluding null terminator).
//! This declaration also reserves space to hold the string value when the
//! metric is written.
#define MEMFAULT_METRICS_STRING_KEY_DEFINE(key_name, max_length) MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Declare a string metric for use in a session. See`MEMFAULT_METRICS_STRING_KEY_DEFINE` for
//! details.
#define MEMFAULT_METRICS_STRING_KEY_DEFINE_WITH_SESSION(key_name, max_length, session_key) \
  MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Declare a metric which is tracked in a metric session.
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION(key_name, value_type, session_key) \
  MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Same as 'MEMFAULT_METRICS_KEY_DEFINE_WITH_SESSION', with range hints specified
#define MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE_AND_SESSION(key_name, value_type, min_value, \
                                                           max_value, session_key)          \
  MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Define a metric session.
//!
//! A metric session is analogous to a heartbeat, but you can use it to track metrics
//! across an arbitrary event. For example, if you wanted to track battery life over the course of
//! an LTE session, you could start a session when the LTE modem is powered on and stop the session
//! when the modem is powered off.
#define MEMFAULT_METRICS_SESSION_KEY_DEFINE(key_name) MEMFAULT_METRICS_KEY_DEFINE_TRAP_()

//! Uses a metric key. Before you can use a key, it should defined using MEMFAULT_METRICS_KEY_DEFINE
//! in memfault_metrics_heartbeat_config.def.
//! @param key_name The name of the key, without quotes, as defined using
//! MEMFAULT_METRICS_KEY_DEFINE.
#define MEMFAULT_METRICS_KEY(key_name) _MEMFAULT_METRICS_ID(key_name)

typedef struct MemfaultMetricsBootInfo {
  //! The number of times the system has rebooted unexpectedly since reporting the last heartbeat
  //!
  //! If you do not already have a system in place to track this, consider using the memfault
  //! reboot_tracking module (https://mflt.io/2QlOlgH). This info can be collected by
  //! passing the value returned from memfault_reboot_tracking_get_crash_count().
  //! When using this API we recommend clearing the crash when the first heartbeat since boot
  //! is serialized by implementing memfault_metrics_heartbeat_collect_data() and calling
  //! memfault_reboot_tracking_reset_crash_count().
  //!
  //! If any reboot is unexpected, initialization can also reduce to:
  //!   sMemfaultMetricBootInfo info = { .unexpected_reboot_count = 1 };
  uint32_t unexpected_reboot_count;
} sMemfaultMetricBootInfo;

//! Initializes the metric events API.
//! All heartbeat values will be initialized to their reset values.
//! Integer types will be reset to unset/null.
//! Timer metrics will be reset to 0.
//! String metrics will be reset to empty strings.
//! @param storage_impl The storage location to serialize metrics out to
//! @param boot_info Info added to metrics to facilitate computing aggregate statistics in
//!  the Memfault cloud
//! @note Memfault will start collecting metrics once this function returns.
//! @return 0 on success, else error code
int memfault_metrics_boot(const sMemfaultEventStorageImpl *storage_impl,
                          const sMemfaultMetricBootInfo *boot_info);

//! Set the value of a signed integer metric.
//!
//! Integer metrics that are unset during a heartbeat interval
//! are sent as null and dropped when received.
//! @param key The key of the metric. @see MEMFAULT_METRICS_KEY
//! @param value The new value to set for the metric
//! @return 0 on success, else error code
//! @note The metric must be of type kMemfaultMetricType_Signed
int memfault_metrics_heartbeat_set_signed(MemfaultMetricId key, int32_t signed_value);

//! Same as @memfault_metrics_heartbeat_set_signed except for a unsigned integer metric
int memfault_metrics_heartbeat_set_unsigned(MemfaultMetricId key, uint32_t unsigned_value);

//! Set the value of a string metric.
//! @param key The key of the metric. @see MEMFAULT_METRICS_KEY
//! @param value The new value to set for the metric
//! @return 0 on success, else error code
//! @note The metric must be of type kMemfaultMetricType_String
int memfault_metrics_heartbeat_set_string(MemfaultMetricId key, const char *value);

//! Used to start a "timer" metric
//!
//! Timer metrics can be useful for tracking durations of events which take place while the
//! system is running. Some examples:
//!  - time a task was running
//!  - time spent in different power modes (i.e run, sleep, idle)
//!  - amount of time certain peripherals were running (i.e accel, bluetooth, wifi)
//!
//! @return 0 if starting the metric was successful, else error code.
int memfault_metrics_heartbeat_timer_start(MemfaultMetricId key);

//! Same as @memfault_metrics_heartbeat_start but *stops* the timer metric
//!
//! @return 0 if stopping the timer was successful, else error code
int memfault_metrics_heartbeat_timer_stop(MemfaultMetricId key);

//! Add the value to the current value of a metric.
//! @param key The key of the metric. @see MEMFAULT_METRICS_KEY
//! @param inc The amount to increment the metric by
//! @return 0 on success, else error code
//! @note The metric must be of type kMemfaultMetricType_Unsigned or kMemfaultMetricType_Signed
int memfault_metrics_heartbeat_add(MemfaultMetricId key, int32_t amount);

//! Alternate API that includes the 'MEMFAULT_METRICS_KEY()' expansion
#define MEMFAULT_METRIC_SET_SIGNED(key_name, signed_value) \
  memfault_metrics_heartbeat_set_signed(MEMFAULT_METRICS_KEY(key_name), (signed_value))
#define MEMFAULT_METRIC_SET_UNSIGNED(key_name, unsigned_value) \
  memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(key_name), (unsigned_value))
#define MEMFAULT_METRIC_SET_STRING(key_name, value) \
  memfault_metrics_heartbeat_set_string(MEMFAULT_METRICS_KEY(key_name), (value))
#define MEMFAULT_METRIC_TIMER_START(key_name) \
  memfault_metrics_heartbeat_timer_start(MEMFAULT_METRICS_KEY(key_name))
#define MEMFAULT_METRIC_TIMER_STOP(key_name) \
  memfault_metrics_heartbeat_timer_stop(MEMFAULT_METRICS_KEY(key_name))
#define MEMFAULT_METRIC_ADD(key_name, amount) \
  memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(key_name), (amount))

//! For debugging purposes: prints the current heartbeat values using
//! MEMFAULT_LOG_DEBUG(). Before printing, any active timer values are computed.
//! Other metrics will print the current values. This can be called from the
//! user-supplied memfault_metrics_heartbeat_collect_data() function to print
//! values set there (use memfault_metrics_heartbeat_debug_trigger() to trigger
//! a metrics collection).
void memfault_metrics_heartbeat_debug_print(void);

//! For debugging purposes: triggers the heartbeat data collection handler, as if the heartbeat
//! timer had fired. We recommend also testing that the heartbeat timer fires by itself. To get the
//! periodic data collection triggering rapidly for testing and debugging, consider using a small
//! value for MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS.
void memfault_metrics_heartbeat_debug_trigger(void);

//! For debugging and unit test purposes, allows for the extraction of different values
int memfault_metrics_heartbeat_read_unsigned(MemfaultMetricId key, uint32_t *read_val);
int memfault_metrics_heartbeat_read_signed(MemfaultMetricId key, int32_t *read_val);
int memfault_metrics_heartbeat_timer_read(MemfaultMetricId key, uint32_t *read_val);
int memfault_metrics_heartbeat_read_string(MemfaultMetricId key, char *read_val,
                                           size_t read_val_len);

//! Callback used to collect custom metrics at the end of a session.
//!
//! This CB is called when a session is ended. It can be used to collect custom metrics
//! whose value at the end of a session are all that matters. For example, if you wanted to
//! track the battery life over the course of an LTE session.
typedef void (*MemfaultMetricsSessionEndCb)(void);

//! Register a callback to be called when a session is ended.
//!
//! @param session_key The key of the session to register the callback for
//! @param session_end_cb The callback to be called when the session has ended
void memfault_metrics_session_register_end_cb(eMfltMetricsSessionIndex session_key,
                                              MemfaultMetricsSessionEndCb session_end_cb);

//! Used to start a metric "session".
//!
//! A session is a special type of metric which tracks the time a certain event is active.
//! For example, if you wanted to track battery life over the course of an LTE session, you could
//! start a session when the LTE modem is powered on and stop the session when the modem is
//! powered off.
int memfault_metrics_session_start(eMfltMetricsSessionIndex session_key);

//! Used to stop a metric "session".
//!
//! Same as @memfault_metrics_session_start except for stopping a session.
int memfault_metrics_session_end(eMfltMetricsSessionIndex session_key);

//! Alternate API that includes the 'MEMFAULT_METRICS_SESSION_KEY()' expansion
#define MEMFAULT_METRICS_SESSION_START(key) \
  memfault_metrics_session_start(MEMFAULT_METRICS_SESSION_KEY(key))
#define MEMFAULT_METRICS_SESSION_END(key) \
  memfault_metrics_session_end(MEMFAULT_METRICS_SESSION_KEY(key))

//! Collect built-in metrics as part of default ports in memfault-firmware-sdk.
//!
//! It can (optionally) be overridden by a port to collect a set of built-in metrics
//! as configured by the port/application.
//!
//! @note By default, a weak version of this function is implemented which is empty
//! @note This API is for internal use only and should never be called by an end user
void memfault_metrics_heartbeat_collect_sdk_data(void);

#ifdef __cplusplus
}
#endif
