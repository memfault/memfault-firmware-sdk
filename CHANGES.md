### Changes between Memfault SDK 0.9.3 and SDK 0.9.2 - Dec 14, 2020

#### :chart_with_upwards_trend: Improvements

- nRF Connect Upates:

  - Updated port to support
    [nRF Connect SDK v1.4.0](https://github.com/nrfconnect/sdk-nrf/tree/v1.4-branch)
  - Added port of HTTP client to post data to Memfault
  - Added support for capturing state of all Zephyr tasks during a crash. This
    way the state of all threads can be seen in the Memfault UI when a crash is
    uploaded.
  - Updated
    [memfault_demo_app](platforms/nrf-connect-sdk/nrf9160/memfault_demo_app) to
    use the nRF9160-DK
  - Added notes to the
    [step-by-step integration guide](https://mflt.io/nrf-connect-sdk-integration-guide)
    for the nRF9160.

### Changes between Memfault SDK 0.9.2 and SDK 0.9.1 - Dec 10, 2020

#### :chart_with_upwards_trend: Improvements

- Added Memfault OTA support to esp-idf port. Updates can now be performed by
  calling `memfault_esp_port_ota_update()`. More details can be found in
  [`ports/esp_idf/memfault/include/memfault/esp_port/http_client.h`](ports/esp_idf/memfault/include/memfault/esp_port/http_client.h)
- The esp-idf port debug CLI can now easily be disabled by using the
  `MEMFAULT_CLI_ENABLED=n` Kconfig option.
- Added FreeRTOS utility to facilitate collecting minimal set of RAM in a
  coredump necessary to recover backtraces for all tasks. More details can be
  found in
  [`ports/freertos/include/memfault/ports/freertos_coredump.h`](ports/freertos/include/memfault/ports/freertos_coredump.h)
- Previously, if the Memfault event storage buffer was out of space, a "storage
  out of space" error would be printed every time. Now, an error message is
  printed when the issue first happend and an info message is printed when space
  is free again.
- Added a reference software watchdog port for the STM32H7 series LPTIM
  peripheral. Users of the STM32 HAL can now compile in the reference port and
  the `MemfaultWatchdog_Handler`. The handler will save a coredump so the full
  system state can be recovered when a watchdog takes place. More details can be
  found in
  [`ports/include/memfault/ports/watchdog.h`](ports/include/memfault/ports/watchdog.h).

### Changes between Memfault SDK 0.9.1 and SDK 0.9.0 - Nov 24, 2020

#### :chart_with_upwards_trend: Improvements

- A log can now be captured alongside a trace event by using a new API:
  [`MEMFAULT_TRACE_EVENT_WITH_LOG(reason, ...)`](components/core/include/memfault/core/trace_event.h#L77).
  This can be useful to capture arbitrary diagnostic data with an error event or
  to capture critical error logs that you would like to be alerted on when they
  happen. For example:

```c
// @file memfault_trace_reason_user_config.def
MEMFAULT_TRACE_REASON_DEFINE(Critical_Log)
```

```c
// @file your_platform_log_implementation.h
#include "memfault/core/trace_event.h"

#define YOUR_PLATFORM_LOG_CRITICAL(fmt, ....) \
    MEMFAULT_TRACE_EVENT_WITH_LOG(Critical_Log, fmt, __VA_ARGS__)
```

```c
// @file your_platform_temperature_driver.c
void record_temperature(void) {
   // ...
   // erase flash to free up space
   int rv = spi_flash_erase(...);
   if (rv != 0) {
      YOUR_PLATFORM_LOG_CRITICAL("Flash Erase Failure: rv=%d, spi_err=0x%x", spi_bus_get_status());
   }
}
```

- The error tracing facilities are now initialized automatically for the esp-idf
- Fixed a :bug: where an erroneous size was reported from
  `memfault_coredump_storage_check_size()` if
  `MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS=1`and `memfault_log_boot()` had not yet
  been called

### Changes between Memfault SDK 0.9.0 and SDK 0.8.2 - Nov 16, 2020

#### :chart_with_upwards_trend: Improvements

- ESP32 port improvements:
  - The Memfault `metrics` component is now included by default in the ESP32
    port
  - `MEMFAULT_LOG_DEBUG` messages now print by default
  - Added a `heartbeat_dump` CLI command for easy viewing of current heartbeat
    metrics
  - Custom handling of collecting Memfault data can now easily be implemented in
    the ESP32 port using the new
    [`memfault_esp_port_data_available()` & `memfault_esp_port_get_chunk()`](ports/esp_idf/memfault/include/memfault/esp_port/core.h)
    APIS. This can be useful in scenarios where there are external MCUs
    forwarding Memfault chunks to the ESP32.
- The platform port for the memfault log dependency can now be implemented by
  macros (rather than the `memfault_platform_log` dependency). See
  [`components/core/include/memfault/core/debug_log.h`](components/core/include/memfault/core/debug_log.h)
  for more details.

#### :boom: Breaking Changes

- If you were using the ESP32 port:
  - Call to `memfault_metrics_boot()` can now be removed
  - Custom implementations for `memfault_platform_metrics_timer_boot()` &
    `memfault_platform_get_time_since_boot_ms()` can be removed as they are now
    provided as part of the port.

### Changes between Memfault SDK 0.8.2 and SDK 0.8.1 - Nov 13, 2020

#### :chart_with_upwards_trend: Improvements

- Coredumps will now be truncated (instead of failing to save completely) when
  the memory regions requested take up more space than the platform storage
  allocated for saving. A warning will also be displayed in the Memfault UI when
  this happens. Regions are always read in the order returned from
  [`memfault_platform_coredump_get_regions()`](components/panics/include/memfault/panics/platform/coredump.h)
  so it is recommended to order this list from the most to least important
  regions to capture.
- Updated FreeRTOS port to use static allocation APIs by default when the
  `configSUPPORT_STATIC_ALLOCATION=1` configuration is used.

### Changes between Memfault SDK 0.8.1 and SDK 0.8.0 - Nov 3, 2020

#### :chart_with_upwards_trend: Improvements

- Added several more
  [reboot reason options](components/panics/include/memfault/core/reboot_reason_types.h#L16):
  `kMfltRebootReason_SoftwareReset` & `kMfltRebootReason_DeepSleep`.
- Extended [ESP32 port](https://mflt.io/esp-tutorial) to include integrations
  for [reboot reason tracking](https://mflt.io/reboot-reasons) and
  [log collection](https://mflt.io/logging).
- Apply missing check to unit test
  [reported on Github](https://github.com/memfault/memfault-firmware-sdk/pull/6)

### Changes between Memfault SDK 0.8.0 and SDK 0.7.2 - Oct 26, 2020

#### :chart_with_upwards_trend: Improvements

- Added a new convenience API,
  [`memfault_coredump_storage_check_size()`](components/panics/include/memfault/panics/coredump.h),
  to check that coredump storage is appropriately sized.
- Fixed a :bug: with heartbeat timers that would lead to an incorrect duration
  being reported if the timer was started and stopped within the same
  millisecond.
- Fixed an issue when using TI's compiler that could lead to the incorrect
  register state being captured during a fault.

#### :boom: Breaking Changes

- If you were **not** using the
  [error tracing functionality](https://mflt.io/error-tracing), you will need to
  create the configuration file "memfault_trace_reason_user_config.def" and add
  it to your include path. This removes the requirement to manually define
  `MEMFAULT_TRACE_REASON_USER_DEFS_FILE` as part of the compiler flags.

### Changes between Memfault SDK 0.7.3 and SDK 0.7.2 - Oct 5, 2020

#### :chart_with_upwards_trend: Improvements

- Add support for sending multiple events in a single chunk. This can be useful
  for optimizing throughput or packing more data into a single transmission
  unit. The behavior is disabled by default but can be enabled with the
  `MEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED` compiler flag. More details can
  be found in
  [memfault_event_storage.c](components/core/src/memfault_event_storage.c#L30)
- Added convenience API, `memfault_build_id_get_string`, for populating a buffer
  with a portion of the
  [Memfault Build ID](components/core/include/memfault/core/build_info.h#L8-L42)
  as a string.
- Added default implementations of several Memfault SDK dependency functions
  when using FreeRTOS to [ports/freertos](ports/freertos).

### Changes between Memfault SDK 0.7.2 and SDK 0.7.1 - Sept 1, 2020

#### :chart_with_upwards_trend: Improvements

- A status or error code (i.e bluetooth disconnect reason, errno value, etc) can
  now be logged alongside a trace event by using a new API:
  [`MEMFAULT_TRACE_EVENT_WITH_STATUS(reason, status_code)`](components/core/include/memfault/core/trace_event.h#L55).

### Changes between Memfault SDK 0.7.1 and SDK 0.7.0 - Sept 1, 2020

#### :chart_with_upwards_trend: Improvements

- Added support for TI's ARM-CGT Compiler
- Removed dependency on NMI Exception Handler for `MEMFAULT_ASSERT`s. Instead of
  pending an NMI exception, the assert path will now "trap" into the fault
  handler by executing a `udf` instruction. This unifies the fault handling
  paths within the SDK and leaves the NMI Handler free for other uses within the
  user's environment.
- Added several more
  [reboot reason options](components/panics/include/memfault/core/reboot_reason_types.h#L16):
  `kMfltRebootReason_PowerOnReset`, `kMfltRebootReason_BrownOutReset`, &
  `kMfltRebootReason_Nmi`.

### Changes between Memfault SDK 0.7.0 and SDK 0.6.1 - Aug 6, 2020

#### :chart_with_upwards_trend: Improvements

- Added utility to facilitate collection of the memory regions used by the
  [logging module](components/core/include/memfault/core/log.h) as part of a
  coredump. With this change, when the SDK is compiled with
  `MEMFAULT_COREDUMP_COLLECT_LOG_REGIONS=1`, the logging region will
  automatically be collected as part of a coredump. Step-by-step details can
  also be found in the [logging integration guide](https://mflt.io/logging).
- Added `MEMFAULT_METRICS_KEY_DEFINE_WITH_RANGE()` which can be used for
  defining the minimum and maximum expected range for a heartbeat metric. This
  information is used by the Memfault cloud to better normalize the data when it
  is presented in the UI.

#### :boom: Breaking Changes

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system) and are using
  the `panics` component, you will need to manually add the following file to
  your build system:
  `$(MEMFAULT_SDK_ROOT)/components/panics/src/memfault_coredump_sdk_regions.c`

### Changes between Memfault SDK 0.6.1 and SDK 0.6.0 - July 27, 2020

#### :chart_with_upwards_trend: Improvements

- Added a port for projects using the [nRF Connect SDK](ports/nrf-connect-sdk)
  along with a
  [step-by-step integration guide](https://mflt.io/nrf-connect-sdk-integration-guide).
- Disabled optimizations for `memfault_data_export_chunk()` to guarantee the
  [GDB chunk test utility](https://mflt.io/send-chunks-via-gdb) can always be
  used to post chunks using the data export API.

### Changes between Memfault SDK 0.6.0 and SDK 0.5.1 - July 21, 2020

#### :rocket: New Features

- Added
  [memfault/core/data_export.h](components/core/include/memfault/core/data_export.h#L5)
  API to facilitate production and evaluation use cases where Memfault data is
  extracted over a log interface (i.e shell, uart console, log file, etc). See
  the header linked above or the
  [integration guide](https://mflt.io/chunk-data-export) for more details.

#### :chart_with_upwards_trend: Improvements

- Fixed a :bug: that would cause the demo shell to get stuck if backspace
  chracters were entered while no other characters had been entered.
- Updated the [GDB chunk test utility](https://mflt.io/send-chunks-via-gdb) to
  automatically detect when the data export API is integrated and post-chunks to
  the cloud directly from GDB when the function is invoked.

#### :boom: Breaking Changes

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system) and want to make
  use of the new data export API, you will need to manually add the following
  files to your build system:
  - Add: `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_data_export.c`
  - Add: `$(MEMFAULT_SDK_ROOT)/components/util/src/memfault_base64.c`

### Changes between Memfault SDK 0.5.1 and SDK 0.5.0 - June 24, 2020

#### :chart_with_upwards_trend: Improvements

- Updated code to support compilations with `-Wunused-paramater`, GNU GCC's
  `-Wformat-signedness`, and Clang's `-Wno-missing-prototypes` &
  `-Wno-missing-variable-declarations`.
- Updated unit test setup to compile with newly supported warnings treated as
  errors

#### :house: Internal

- Misc utility additions including support for encoding floats and int64_t's in
  the cbor utility

### Changes between Memfault SDK 0.5.0 and SDK 0.4.2 - June 11, 2020

#### :rocket: New Features

- Add additional utilities to the http component to facilitate easier
  [release management](https://mflt.io/release-mgmt) integration in environments
  with no pre-existing http stack.
- Add new cli command, `mflt get_latest_release`, to the Zephyr demo application
  (tested on the STM32L4) to demonstrate querying the Memfault cloud for new
  firmware updates.

#### :chart_with_upwards_trend: Improvements

- Refactored `demo` component to make it easier to integrate an individual CLI
  commands into a project since some of the commands can be helpful for
  validating integrations. More details can be found in the README at
  [components/demo/README.md](components/demo/README.md).

#### :boom: Breaking Changes

- If you are using the "demo" component _and_ are _not_ making use our CMake or
  Make [build system helpers](README.md#add-sources-to-build-system), you will
  need to make the following changes:
  - Update:
    `$(MEMFAULT_SDK_ROOT)/components/demo/src/{memfault_demo_cli.c => panics/memfault_demo_panics.c}`
  - Update:
    `$(MEMFAULT_SDK_ROOT)/components/demo/src/{ => panics}/memfault_demo_cli_aux.c`
  - Add: `$(MEMFAULT_SDK_ROOT)/components/demo/src/memfault_demo_core.c`
  - Add: `$(MEMFAULT_SDK_ROOT)/components/demo/src/http/memfault_demo_http.c`
- If you are using the `http` component, the following macro names changed:

```diff
-#define MEMFAULT_HTTP_GET_API_PORT()
-#define MEMFAULT_HTTP_GET_API_HOST()
+#define MEMFAULT_HTTP_GET_CHUNKS_API_PORT()
+#define MEMFAULT_HTTP_GET_CHUNKS_API_HOST()
```

### Changes between Memfault SDK 0.4.2 and SDK 0.4.1 - June 8, 2020

#### :chart_with_upwards_trend: Improvements

- Moved `reboot_tracking.h` to `core` component since it has no dependencies on
  anything from the `panics` component. This allows the reboot tracking to be
  more easily integrated in a standalone fashion.
- [Published a new guide](https://mflt.io/release-mgmt) detailing how to manage
  firmware updates using Memfault.
- Disabled optimizations for `memfault_fault_handling_assert()`. This improves
  the recovery of local variables of frames in the backtrace when certain
  optimization levels are used.
- Updated `memfault_sdk_assert.c` to address a GCC warning
  (`-Wpointer-to-int-cast`) emitted when compiling the file for 64 bit
  architectures.
- Misc README improvements.

#### :boom: Breaking Changes

- If you are already using reboot tracking in your system, you will need to
  update the following includes in your code:

```diff
-#include "memfault/panics/reboot_tracking.h"
-#include "memfault/panics/reboot_reason_types.h"
+#include "memfault/core/reboot_tracking.h"
+#include "memfault/panics/reboot_reason_types.h"
```

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to update the path for the following files:

  - `$(MEMFAULT_SDK_ROOT)/components/{panics => core}/src/memfault_ram_reboot_info_tracking.c`
  - `$(MEMFAULT_SDK_ROOT)/components/{panics => core}/src/memfault_reboot_tracking_serializer.c`

- `eMfltResetReason` was renamed to `eMemfaultRebootReason`.

### Changes between Memfault SDK 0.4.1 and SDK 0.4.0 - May 20, 2020

#### :rocket: New Features

- Added native SDK support for tracking and generating a unique firmware build
  id with any compiler! Quick integration steps can be found in
  [memfault/core/build_info.h](components/core/include/memfault/core/build_info.h#L8-L42).
  It is very common, especially during development, to not change the firmware
  version between editing and compiling the code. This will lead to issues when
  recovering backtraces or symbol information because the debug information in
  the symbol file may be out of sync with the actual binary. Tracking a build id
  enables the Memfault cloud to identify and surface when this happens! You can
  also make use of two new APIs:
  - `memfault_build_info_dump()` can be called on boot to display the build that
    is running. This can be a useful way to sanity check that your debugger
    succesfully flashed a new image.
  - `memfault_build_info_read()` can be used to read the build id for your own
    use cases. For example you could append a portion of it to a debug version
    to make it unique.

#### :chart_with_upwards_trend: Improvements

- The CMake [build system helper](README.md#add-sources-to-build-system) is now
  compatible with any 3.x version of CMake (previously required 3.6 or newer).
- The unique firmware build id is stored automatically as part of coredump
  collection.

#### :boom: Breaking Changes

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to add the following files to your build:
  - `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_build_id.c`
  - `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_core_utils.c`
- We also encourage you to add a
  [unique build id](components/core/include/memfault/core/build_info.h#L8-L42)
  to your build (several line code change).

### Changes between Memfault SDK 0.4.0 and SDK 0.3.4 - May 6, 2020

#### :rocket: New Features

- Added support for (optionally) storing events collected by the SDK to
  non-volatile storage mediums. This can be useful for devices which experience
  prolonged periods of no connectivity. To leverage the feature, an end user
  must implement the
  [nonvolatile_event_storage platform API](components/core/include/memfault/core/platform/nonvolatile_event_storage.h#L7).

#### :chart_with_upwards_trend: Improvements

- Added an assert used internally by the SDK which makes it easier to debug API
  misuse during bringup. The assert is enabled by default but can easily be
  disabled or overriden. For more details see
  [`memfault/core/sdk_assert.h`](components/core/include/memfault/core/sdk_assert.h#L6).
- Added a default implementation of
  [`memfault_platform_halt_if_debugging()`](components/core/src/arch_arm_cortex_m.c#L20-L34)
  for Cortex-M targets. The function is defined as _weak_ so a user can still
  define the function to override the default behavior.

#### :house: Internal

- Updated
  [`memfault install_chunk_handler`](https://mflt.io/posting-chunks-with-gdb) to
  work with older versions of the GNU Arm Toolchain.

#### :boom: Breaking Changes

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to add `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_sdk_assert.c` to
  your project.

### Changes between Memfault SDK 0.3.4 and SDK 0.3.3 - April 22, 2020

#### :chart_with_upwards_trend: Improvements

- Moved `trace_event.h` to `core` component since it has no dependencies on
  anything from the `panics` component. This allows the trace event feature to
  be more easily integrated in a standalone fashion.

#### :boom: Breaking Changes

- If you are already using `MEMFAULT_TRACE_EVENT()` in your project, you will
  need to update the include as follows:

```diff
-#include "memfault/panics/trace_event.h"
+#include "memfault/core/trace_event.h"
```

- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to update the source path for `components/panics/src/memfault_trace_event.c`
  to `components/core/src/memfault_trace_event.c`

### Changes between Memfault SDK 0.3.3 and SDK 0.3.2 - April 21, 2020

#### :rocket: New Features

- Added a new [GDB command](https://mflt.io/posting-chunks-with-gdb) which can
  be used to post packetized Memfault data directly from GDB to the Memfault
  cloud. This can be helpful as a way to quickly test data collection
  functionality while working on an integration of the SDK.

### Changes between Memfault SDK 0.3.2 and SDK 0.3.1 - April 16, 2020

#### :rocket: New Features

- The `captured_date` for an event can now be set by implementing
  [`memfault_platform_time_get_current()`](components/core/include/memfault/core/platform/system_time.h#L33).
  If the API is not implemented, the `captured_date` will continue to be set
  based on the time the event was received by the memfault cloud.

#### :chart_with_upwards_trend: Improvements

- Added a reference implementation of
  [reboot reason tracking](https://mflt.io/2QlOlgH) to the
  [NRF52 demo app](platforms/nrf5/libraries/memfault/platform_reference_impl/memfault_platform_reboot_tracking.c#L1)
  and a new `reboot` CLI command to easily exercise it.
- A `reset_reason` can now optionally be provided as part of
  [`sResetBootupInfo`](components/panics/include/memfault/core/reboot_tracking.h#L41).
  This can be useful for scenarios where the reboot reason is known on bootup
  but could not be set prior to the device crashing.
- A reboot reason event will now _always_ be generated when
  `memfault_reboot_tracking_boot()` is called even if no information about the
  reboot has been provided. In this scenario, the reset reason will be
  [`kMfltRebootReason_Unknown`](components/panics/include/memfault/core/reboot_reason_types.h#L16)

#### :house: Internal

- Updated a few Kconfig options in the Zephyr demo app to improve the ability to
  compute stack high water marks (`CONFIG_INIT_STACKS=y`) and determine if
  stacks has overflowed (`CONFIG_MPU_STACK_GUARD=y`).

#### :boom: Breaking Changes

- `device_serial` is no longer encoded by default as part of events. Instead,
  the `device_serial` in an event is populated from the the unique device
  identifier used when posting the data to the
  [chunks REST endpoint](https://mflt.io/chunks-api). This leads to ~20%
  reduction in the size of a typical event. Encoding `device_serial` as part of
  the event itself can still be enabled by adding
  [`-DMEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL=1`](components/core/src/memfault_serializer_helper.c#L23)
  as a compilation flag but should not be necessary for a typical integration.

### Changes between Memfault SDK 0.3.1 and SDK 0.3.0 - April 9, 2020

#### :rocket: New Features

- Added
  [`memfault_log_read()`](components/core/include/memfault/core/log.h#L95-L121)
  API to make it possible to use the module to cache logs in RAM and then flush
  them out to slower mediums, such as a UART console or Flash, from a background
  task.

#### :chart_with_upwards_trend: Improvements

- A pointer to the stack frame upon exception entry is now included in
  `sCoredumpCrashInfo` when
  [memfault_platform_coredump_get_regions](components/panics/include/memfault/panics/platform/coredump.h#L56)
  is invoked. This can (optionally) be used to configure regions collected based
  on the state or run platform specific cleanup based on the state.
- Added Root Certificates needed for release downloads to
  [`MEMFAULT_ROOT_CERTS_PEM`](components/http/include/memfault/http/root_certs.h#L146).

#### :house: Internal

- All sources that generate events now use the same utility function,
  `memfault_serializer_helper_encode_metadata()` to encode common event data.

### Changes between Memfault SDK 0.3.0 and SDK 0.2.5 - April 3, 2020

#### :rocket: New Features

- Introduced a lightweight logging module. When used, the logs leading up to a
  crash will now be decoded and displayed from the Memfault Issue Details Web
  UI. For instructions about how to use the module in your project, check out
  [log.h](components/core/include/memfault/core/log.h).
- The metrics component will now automatically collect the elapsed time and the
  number of unexpected reboots during a heartbeat interval. The Memfault cloud
  now uses this information to automatically compute and display the overall
  uptime of your fleet.

#### :chart_with_upwards_trend: Improvements

- Added a [new document](https://mflt.io/fw-event-serialization) walking through
  the design of Memfault event serialization.
- Cleaned up `test_memfault_root_cert.cpp` and moved the `base64` implementation
  within the unit test to the `util` component so it is easier to share the code
  elsewhere in the future.
- Added `const` to a few circular_buffer.h API signatures.
- Misc code comment improvements.

#### :boom: Breaking Changes

- The function signature for `memfault_metrics_boot()` changed as part of this
  update. If you are already using the `metrics` component, you will need to
  update the call accordingly. See the notes in
  [metrics.h](components/metrics/include/memfault/metrics/metrics.h) for more
  details.
- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to add `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_log.c` to your
  project. (Note that until
  [`memfault_log_boot()`](https://github.com/memfault/memfault-firmware-sdk/blob/master/components/core/include/memfault/core/log.h#L33-L38)
  is called, all calls made to the logging module will be a no-op).

### Changes between Memfault SDK 0.2.5 and SDK 0.2.4 - March 20, 2020

- Improve the `memfault_platform_coredump_storage_clear()` NRF52 reference
  implementation for situations where the SoftDevice is enabled and there is a
  lot of Bluetooth activity. (In this scenario, NRF52 flash operations may need
  retries or take a while to complete).
- Fixed compiler error that could arise with the metrics component when using
  Arm Compiler 5 due to multiply defined weak symbols.

### Changes between Memfault SDK 0.2.4 and SDK 0.2.3 - March 10, 2020

- Add support for ESP32 (Tensilica Xtensa LX6 MCU) to the **panics** component.
  - A step-by-step integration guide can be found
    [here](https://mflt.io/esp-tutorial).
  - A drop-in port for an existing v3.x or v4.x based ESP-IDF project can be
    found at [ports/esp_idf](ports/esp_idf).
  - An example application exercising the memfault-firmware-sdk can be found
    [here](platforms/esp32/README.md).

### Changes between Memfault SDK 0.2.3 and SDK 0.2.2 - March 3, 2020

- If a [software watchdog](https://mflt.io/root-cause-watchdogs) has been
  implemented on a Cortex-M device, `MemfaultWatchdog_Handler` can now be
  registered as the Exception Handler to automatically collect a coredump.
- For heartbeat metrics, instead of serializing the name of each metric, we now
  recover it from the debug information in the symbol file in the Memfault
  cloud. For a typical heartbeat this reduces the serialization size by more
  than 50% and results in a smaller footprint than other structured
  serialization alternatives such as Protobuf.
- Remove usage of `__has_include` macro for IAR compiler since not all versions
  fully support it.

### Changes between Memfault SDK 0.2.1 and SDK 0.2.2 - Feb 20, 2020

- Add support for calling `MEMFAULT_TRACE_EVENT()` from interrupts. Note: If you
  are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), this change
  requires that you add
  `$(MEMFAULT_SDK_ROOT)/components/core/src/arch_arm_cortex_m.c` to your
  project.
- Misc documentation improvements.

### Changes between Memfault SDK 0.2.0 and SDK 0.2.1 - Feb 14, 2020

- Add support for compressing coredumps as they are sent using Run Length
  Encoding (RLE). More details can be found in
  [memfault/core/data_source_rle.h](components/core/include/memfault/core/data_source_rle.h).
- Update **metrics** component to support compilation with the IAR ARM C/C++
  Compiler.
- Update Mbed OS 5 port to use `memfault_demo_shell` instead `mbed-client-cli`,
  since `mbed-client-cli` is not part of the main Mbed OS 5 distribution.
- Update nrf52 example application to only collect the active parts of the stack
  to reduce the overall size of the example coredump.

### Changes between Memfault SDK 0.2.0 and SDK 0.1.0 - Feb 5, 2020

- Add a new API ("Trace Event") for tracking errors. This allows for tracing
  issues that are unexpected but are non-fatal (don't result in a device
  reboot). A step-by-step guide detailing how to use the feature can be found
  at: https://mflt.io/error-tracing
- Update GDB coredump collection script to work with the ESP32 (Tensilica Xtensa
  LX6 Architecture)
- Remove `__task` from IAR Cortex-M function handler declarations since it's not
  explicitly required and can lead to a compiler issue if the function prototype
  does not also use it.
- Misc documentation and comment tweaks to make nomenclature more consistent

### Changes between Memfault SDK 0.1.0 and SDK 0.0.18 - Jan 22, 2020

- Update **panics** component to support compilation with IAR ARM C/C++
  Compiler. More details about integrating IAR can be found at
  https://mflt.io/iar-tutorial. As part of the change `MEMFAULT_GET_LR()` now
  takes an argument which is the location to store the LR to
  (`void *lr = MEMFAULT_GET_LR()` -> `void *lr;` `MEMFAULT_GET_LR(lr)`)

### Changes between Memfault SDK 0.0.18 and SDK 0.0.17 - Jan 14, 2020

- Update the chunk protocol to encode CRC in last chunk of message instead of
  first. This allows the CRC to be computed incrementally and the underlying
  message to be read once (instead of twice). It also makes it easier to use the
  data packetizer in environments where reads from data sources need to be
  performed asynchronously. More details can be found at
  https://mflt.io/data-to-cloud
- Fixed a couple documentation links

### Changes between Memfault SDK 0.0.17 and SDK 0.0.16 - Jan 7, 2020

- Guarantee that all accesses to the platform coredump storage region route
  through `memfault_coredump_read` while the system is running.
- Scrub unused portion of out buffer provided to packetizer with a known pattern
  for easier debugging

### Changes between Memfault SDK 0.0.16 and SDK 0.0.15 - Jan 6, 2020

- Add convenience API, `memfault_packetizer_get_chunk()`, to
  [data_packetizer](components/core/include/memfault/core/data_packetizer.h)
  module.
- Add a new eMfltCoredumpRegionType, `MemoryWordAccessOnly` which can be used to
  force the region to be read 32 bits at a time. This can be useful for
  accessing certain peripheral register ranges which are not byte addressable.
- Automatically collect Exception / ISR state for Cortex-M based targets. NOTE
  that the default config requires an additional ~150 bytes of coredump storage.
  The feature can be disabled completely by adding
  `-DMEMFAULT_COLLECT_INTERRUPT_STATE=0` to your compiler flags. More
  configuration options can be found in
  [memfault_coredump_regions_armv7.m](components/panics/src/memfault_coredump_regions_armv7.c).
- Improve documentation about integrating the SDK within a project in README
- Update Release note summary to use markdown headings for easier referencing.
- Update try script used to collect coredumps via GDB to also collect
  Exception/ISR register information.

### Changes between Memfault SDK 0.0.15 and SDK 0.0.14 - Dec 19, 2019

- Add ARMv6-M fault handling port to **panics** component for MCUs such as the
  ARM Cortex-M0 and Cortex-M0+.

### Changes between Memfault SDK 0.0.14 and SDK 0.0.13 - Dec 18, 2019

- Update **panics** component to support compilation with Arm Compiler 5.

### Changes between Memfault SDK 0.0.13 and SDK 0.0.12 - Dec 16, 2019

- Updated Cortex-M fault handler in **panics** component to also collect `psp`
  and `msp` registers when the system crashes. This allows for the running
  thread backtrace to be more reliably recovered when a crash occurs from an
  ISR.
- Added optional `MEMFAULT_EXC_HANDLER_...` preprocessor defines to enable
  custom naming of exception handlers in the **panics** component.
- Add port for Cortex-M based targets using Quantum Leaps' QP™/C & QP™/C++
  real-time embedded framework. See [ports/qp/README.md](ports/qp/README.md) for
  more details.
- Add demo application running Quantum Leaps' QP™/C running on the
  [STM32F407 discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html).
  See [platforms/qp/README.md](platforms/qp/README.md) for more details.
- Added demo application and port for Arm Mbed OS 5 running on the
  [STM32F429I-DISC1 evaluation board](https://www.st.com/en/evaluation-tools/32f429idiscovery.html).
  See [platforms/mbed/README.md](platforms/mbed/README.md) for more details.
- Changed `print_core` to `print_chunk` in demo applications to better align
  with the Memfault nomenclature for
  [data transfer](https://mflt.io/data-to-cloud).

### Changes between Memfault SDK 0.0.12 and SDK 0.0.11 - Dec 4, 2019

- Expose root certificates used by Memfault CI in DER format for easier
  integration with TLS libraries which do not parse PEM formatted certificates.
- Add utilities to the http component for constructing Memfault cloud **chunk**
  endpoint POST requests to facilitate easier integration in environments with
  no pre-existing http stack.
- Add port for Cortex-M based targets in the Zephyr RTOS. Ports are available
  for the 1.14 Long Term Support Release as well as the 2.0 Release. See
  [ports/zephyr/README.md](ports/zephyr/README.md) for more details
- Add Zephyr demo application (tested on the STM32L4). See
  [zephyr demo app directory](platforms/zephyr/README.md) for more details

### Changes between Memfault SDK 0.0.11 and SDK 0.0.10 - Nov 25, 2019

- Release of **metrics** component. This API can easily be used to monitor
  device health over time (i.e connectivity, battery life, MCU resource
  utilization, hardware degradation, etc) and configure Alerts with the Memfault
  backend when things go astray. To get started, see this
  [document](https://mflt.io/2D8TRLX)

### Changes between Memfault SDK 0.0.10 and SDK 0.0.9 - Nov 22, 2019

- Updated `memfault_platform_coredump_get_regions()` to take an additional
  argument, `crash_info` which conveys information about the crash taking place
  (trace reason & stack pointer at time of crash). This allows platform ports to
  dynamically change the regions collected based on the crash if so desired.
  This will require an update that looks like the following to your port:

```diff
-const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(size_t *num_regions) {
+const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
+    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
```

- Added a new API, `memfault_coredump_storage_compute_size_required()` which can
  be called on boot to sanity check that platform coredump storage is large
  enough to hold a coredump. For example:

```
  sMfltCoredumpStorageInfo storage_info = { 0 };
  memfault_platform_coredump_storage_get_info(&storage_info);
  const size_t size_needed = memfault_coredump_storage_compute_size_required();
  if (size_needed > storage_info.size) {
    MEMFAULT_LOG_ERROR("Coredump storage too small. Got %d B, need %d B",
                       storage_info.size, size_needed);
  }
  MEMFAULT_ASSERT(size_needed <= storage_info.size);
```

- Added a convenience RAM backed
  [reference port](https://github.com/memfault/memfault-firmware-sdk/blob/master/ports/panics/src/memfault_platform_ram_backed_coredump.c)
  for coredump platform APIs. This can be used for persisting a coredump in RAM
  across a reset.

### Changes between Memfault Firmware SDK 0.0.9 and 0.0.8 - Nov 15, 2019

- Enhanced Reboot Tracking module within the **panics** component. Reboots that
  don't result in coredumps can now be easily tracked (i.e brown outs, system
  watchdogs, user initiated resets, resets due to firmware updates, etc). See
  `memfault/core/reboot_tracking.h` for more details and https://mflt.io/2QlOlgH
  for a step-by-step setup tutorial.
- Added Event Storage module within the **core** component. This is a small RAM
  backed data store that queues up traces to be published to the Memfault cloud.
  To minimize the space needed and transport overhead, all events collected
  within the SDK are stored using the Concise Binary Object Representation
  (CBOR)

### Changes between Memfault Firmware SDK 0.0.8 and 0.0.7 - Nov 7, 2019

- Added helper makefile (`makefiles/MemfaultWorker.mk`). A user of the SDK can
  include this makefile when using a make as their build system to easily
  collect the sources and include paths needed to build Memfault SDK components
- Make unit tests public & add CI to external SDK

### Changes between Memfault Firmware SDK 0.0.7 and 0.0.6 - Oct 31, 2019

- Added firmware side support for the Memfault cloud **chunk** endpoint. This is
  a sessionless endpoint that allows chunks of arbitrary size to be sent and
  reassembled in the Memfault cloud. This transport can be used to publish _any_
  data collected by the Memfault SDK. The data is read out on the SDK side by
  calling the `memfault_packetizer_get_next()` API in `data_packetizer.h`. More
  details [here](https://mflt.io/data-to-cloud)
- Updated demo apps to use the new **chunk** endpoint
- Added a new _weak_ function, `memfault_coredump_read()` so platform ports can
  easily add locking on reads to coredump storage if necessary for transmission.
- Updates to this version of the sdk will require the **util** component get
  compiled when using the **panics** API

### Changes between Memfault Firmware SDK 0.0.6 and 0.0.5 - Oct 14, 2019

- Added example port and demo for the STM32H743I-NUCLEO144 evaluation board
  running ChibiOS. See `platforms/stm32/stm32h743i/README.md` for more details.
- general README documentation improvements
- Fixed compilation error that could arise in `memfault_fault_handling_arm.c`
  when using old versions of GCC

### Changes between Memfault Firmware SDK 0.0.5 and 0.0.4 - September 23, 2019

- Updated **panics** SDK to support complex hardware toplogies such as systems
  with multiple MCUs or systems running multiple binaries on a single MCU. More
  details [here](https://mflt.io/34PyNGQ). Users of the SDK will need to update
  the implementation for `memfault_platform_get_device_info()`
- Updated all ports to be in sync with SDK updates

### Changes between Memfault Firmware SDK 0.0.4 and 0.0.3 - August 19, 2019

- Updated **panics** core code to _always_ collect fault registers for ARMv6-M &
  ARMv7-M architectures. The Memfault cloud will auto-analyze these and present
  an analysis.
- Updated https://try.memfault.com gdb script to collect Cortex-M MPU region
  information for auto-analysis
- general README documentation improvements
- improved error reporting strategy and documentation in
  `memfault/core/errors.h`

### Changes between Memfault Firmware SDK 0.0.3 and 0.0.2 - July 2, 2019

- added example port and demo of **panics** SDK for the Nordic nRF52840
  (PCA10056) development kit. See `platforms/nrf5/README.md` for more details.
- Made SDK headers suitable for includion in C++ files

First Public Release of Memfault Firmware SDK 0.0.2 - June 26, 2019

- published initial Memfault SDK, see `README.md` in root for summary
- published **panics** API which is a C SDK that can be integrated into any
  Cortex-M device to save a "core" (crash state) on faults and system asserts.
  See`components/panics/README.md` for more details
- Added example port and demo of **panics** SDK for the BCM943364WCD1 evaluation
  board running the WICED Wifi stack. More details in
  `platforms/wiced/README.md`
- add python invoke based CLI wrapper for demo ports
