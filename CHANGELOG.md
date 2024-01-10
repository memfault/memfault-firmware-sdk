# Memfault Firmware SDK Changelog

## [1.6.0] - 2024-01-09

### :chart_with_upwards_trend: Improvements

- ESP-IDF:

  - In the [ESP32 example app](examples/esp32), disable the shell history cache
    for the FatFS shell command handler, which saves some ~20kB of heap memory.

  - Also in the [ESP32 example app](examples/esp32), add a new built-in metric
    `sync_successful` which is set to `1` when a successful Memfault OTA
    check-in is performed. Primarily added for demonstration purposes for the
    metric. See more information about Memfault Core Metrics
    [here](https://docs.memfault.com/docs/platform/memfault-core-metrics).

- General:

  - Enhanced the Self-Test component with the following new features:

    - Check that other components are initialized correctly (boot test)
    - Check that exporting data over the console is supported (primiarly
      checking that the output buffer is large enough)
    - Print information about the coredump regions that would be collected
      during a crash, for diagnostic purposes

- nRF-Connect SDK:

  - Fix a use-after-free problem in the Memfault FOTA helper code
    ([`ports/zephyr/ncs/src/memfault_fota.c`](ports/zephyr/ncs/src/memfault_fota.c)),
    where the allocated Memfault OTA download URL was freed too early. This
    issue was introduced in Memfault SDK v1.5.0, where support for FOTA on
    nRF-Connect SDK v2.4+ was improved.

- Zephyr:

  - Fix a concurrent access bug in the Memfault Zephyr Logging Backend. Only
    affected configurations with `CONFIG_LOG_MODE_IMMEDIATE=y`. In certain cases
    the logging `ctx` could be corrupted, causing unpredictable behavior.
    Replace the synchronization approach with an atomic primitive and correct a
    potential concurrency issue in the log panic handler.

  - Fix a build warning for certain Zephyr configurations (when using
    `CONFIG_NEWLIB_LIBC=y`) due to a missing declaration for `strnlen`. This
    warning was introduced with the Memfault Self-Test additions in Memfault SDK
    v1.5.2.

### :boom: Breaking Changes

- General:

  - The Battery Metrics platform API has been consolidated into a single
    function,
    `int memfault_platform_get_stateofcharge(sMfltPlatformBatterySoc *soc)`,
    where the platform data is loaded into the `soc` struct. This should
    simplify the platform implementation, and enables platforms to return a
    non-zero value to indicate state-of-charge is unknown. See the new API in
    the header file
    [`memfault/metrics/platform/battery.h`](components/include/memfault/metrics/platform/battery.h)

## [1.5.2] - 2023-12-12

### :chart_with_upwards_trend: Improvements

- General:

  - Add a self-test component which checks the device's integration state. This
    self test is available via the demo cli command `self_test`, or can be
    called directly via `memfault_self_test_run()` (see
    [`self_test.h`](components/include/memfault/core/self_test.h) for details).
    Currently, it validates device info and the build ID.

  - Add a helper macro called `MEMFAULT_REBOOT_MARK_RESET_IMMINENT` in order to
    make marking an imminent reboot easier. It takes the reboot reason as an
    argument.

- ESP-IDF:

  - Add an opt-in assert on malloc failure, controlled with
    `CONFIG_MEMFAULT_ASSERT_ON_ALLOC_FAILURE`. This is useful for tracking heap
    memory issues. Depending on the design, a system may in general not have any
    expected malloc failures, and may not be set up to handle them. This feature
    will generate issues tagged as `Out Of Memory` in the Memfault platform.

- Zephyr:

  - Adjust the implementation of `memfault_zephyr_port_http_upload_sdk_data()`
    to check the socket before each incremental call to the `send()` socket
    operation and wait for the socket to become available. If the socket takes
    too long (a healthy 5 second timeout is given), the function will return
    with an error. Note this function was previously blocking, but may now abort
    mid-transfer. This check was primarily added to prevent a busy loop that
    hogs the CPU, preventing lower priority threads from running. This situation
    could occur with larger HTTP transfers during which the socket may be busy
    processing tx data when another send request occurs.

  - Fix a build error in the Zephyr RTC port.

## [1.5.1] - 2023-12-07

### :chart_with_upwards_trend: Improvements

- nRF-Connect SDK:

  - Correct a build error that occurs when `CONFIG_DOWNLOAD_CLIENT=y` +
    `CONFIG_MEMFAULT_FOTA=n`. This fixes the second issue reported in
    [#66](https://github.com/memfault/memfault-firmware-sdk/issues/66#issuecomment-1845301737)

### :boom: Breaking Changes

- General:

  - The SDK config flag `MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED` now defaults
    to `0`, disabled. This saves a minor amount of code space. Most
    implementations don't need the feature; for users that require it, the flag
    will now need to be set in `memfault_platform_config.h` as
    `#define MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED 1`.

## [1.5.0] - 2023-11-29

### :rocket: New Features

- General:

  - Added support for Session Metrics. These are similar to Heartbeat Metrics,
    except they are set on an arbitrary session interval (not required to be
    aligned to Heartbeat interval). Session metrics are useful for tracking
    device operation sessions- for example, measuring properties for a BLE
    connected stove top during a single cooking session. Session metrics
    automatically include a "session duration" timer metric.

    Session metrics must be defined using the new session-specific APIs, but are
    set using the same `MEMFAULT_METRIC_SET_*` APIs as Heartbeat metrics.

    See the
    [`metrics.h` header file](components/include/memfault/metrics/metrics.h) for
    usage details.

  - New built in metrics for measuring the following properties:

    - **crash-free hours**: enabled by default, generates `operational_hours`
      and `operational_crashfree_hours` metrics, which are automatically
      processed by Memfault

    - **battery drop**: enabled with `#define MEMFAULT_METRICS_BATTERY_ENABLE 1`
      in `memfault_platform_config.h`. See more information in the
      [header file](components/include/memfault/metrics/battery.h) for how to
      use the metric.

    - **connectivity**: enabled with
      `MEMFAULT_METRICS_SYNC_SUCCESS`/`MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS`/`MEMFAULT_METRICS_CONNECTIVITY_CONNECTED_TIME`.
      See more information in the
      [header file](components/include/memfault/metrics/connectivity.h) for how
      to use the metric.

    These metrics are considered first-class metrics by Memfault, are exempt
    from quota limits, and are automatically processed by Memfault.

- Zephyr:

  - By default, set the `sync_memfault_successful`/`sync_memfault_failure`
    metrics for devices using Memfault's Zephyr HTTP chunk upload functionality.
    This feature is controlled with the `CONFIG_MEMFAULT_SYNC_MEMFAULT_METRICS`
    Kconfig flag.

  - Automatically set captured timestamps for events for devices that either
    implement the RTC subsystem, or use the Nordic `date_time` library. The
    appropriate option is enabled by default based on which features are
    available, and can be controlled with the Kconfig flags:
    `CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_DATETIME` or
    `CONFIG_MEMFAULT_SYSTEM_TIME_SOURCE_RTC`.

- ESP-IDF:

  - By default, set the `sync_memfault_successful`/`sync_memfault_failure`
    metrics for devices using Memfault's ESP-IDF HTTP chunk upload
    functionality. This feature is controlled with the
    `CONFIG_MEMFAULT_SYNC_MEMFAULT_METRICS` Kconfig flag.

### :chart_with_upwards_trend: Improvements

- General:

  - Add the ability to extend the Memfault Demo Shell command table with custom
    commands. This is used in the [`examples/freertos`](examples/freertos) demo
    project to add 2 new commands:

    - `freertos_tasks` : print FreeRTOS task information, via `vTaskList()`
    - `freertos_vassert` : trigger a `vAssertCalled` FreeRTOS assert, via
      `configASSERT()`

    The Shell extension API is documented in
    [`components/include/memfault/demo/shell_commands.h`](components/include/memfault/demo/shell_commands.h),
    and must be enabled by setting
    `#define MEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS 1` in
    `memfault_platform_config.h`.

- Zephyr:

  - Remove a warning in Zephyr 3.5+ where the `zephyr/random/rand32.h` header
    was renamed to `zephyr/random/random.h`. This was reported in
    [#66](https://github.com/memfault/memfault-firmware-sdk/issues/66)- thanks
    to @nordicjm for reporting this!

  - Add test commands for exercising Secure Faults in ARM TrustZone-enabled
    chips:

    - `mflt test badptr`
    - `mflt test isr_badptr`

    Note that non-TrustZone chips may not trigger a fault when running those
    commands.

- nRF-Connect SDK:

  - Add the `CONFIG_AT_SHELL` setting to the
    [`examples/nrf-connect-sdk/nrf9160`](nrf-connect-sdk/nrf9160) sample app.
    This permits sending raw AT commands, useful for testing.

  - Specific to nRF-Connect based apps using FOTA, add a warning if
    `CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE > 1024`, which can sporadically error
    out on nRF9160 devices (there is a limitation in the modem, see
    `CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE_2048=y`) and DevZone note
    [here](https://devzone.nordicsemi.com/f/nordic-q-a/68601/tls-2303-bytes-packet-limit/281107).

  - Improve FOTA support for nRF-Connect SDK 2.4+, by improving the technique
    used to find the correct Memfault server root cert. Memfault uses a fast CDN
    to improve OTA payload delivery, which uses a different root cert than the
    Memfault device server. Please contact support@memfault.com immediately if
    you encounter any cert-related issues.

### :boom: Breaking Changes

- The metrics convenience API added in v1.4.3 (`MEMFAULT_HEARTBEAT_SET_*` and
  others) have been renamed to `MEMFAULT_METRIC_SET_*`, to better support the
  new Session Metric feature.

## [1.4.4] - 2023-11-13

### :chart_with_upwards_trend: Improvements

- General:

  - Rename this file from [`CHANGES.md`] to [`CHANGELOG.md`].

  - For FreeRTOS, add a warning when `configRECORD_STACK_HIGH_ADDRESS` is not
    enabled. Memfault uses this to show stack sizes in the coredump view for
    coredumps on FreeRTOS systems. The warning can be disabled by enabling the
    `configRECORD_STACK_HIGH_ADDRESS` FreeRTOS config flag, or by setting
    `#define MEMFAULT_FREERTOS_WARN_STACK_HIGH_ADDRESS_UNAVAILABLE 0`.

  - Make `memfault_packetizer_get_chunk()` return `false` if the buffer was too
    small to load a full chunk. Previously the function would return `true` but
    with `0` bytes loaded into the output buffer and `*buf_len` set to `0`.

  - Update all example Metrics implementations to use the new API from v1.4.3
    (eg `MEMFAULT_METRIC_SET_UNSIGNED` instead of
    `memfault_metrics_heartbeat_set_unsigned`).

  - Fix compilation for systems not integrating the
    [Metrics](https://mflt.io/embedded-metrics) component but using the Demo CLI
    (this regressed in v1.4.3).

## [1.4.3] - 2023-11-08

### :rocket: New Features

- General:

  - Add a new streamlined Metrics setter API:

    - `MEMFAULT_METRIC_SET_SIGNED(key_name, signed_value)`
    - `MEMFAULT_METRIC_SET_UNSIGNED(key_name, unsigned_value)`
    - `MEMFAULT_METRIC_SET_STRING(key_name, value)`
    - `MEMFAULT_METRIC_TIMER_START(key_name)`
    - `MEMFAULT_METRIC_TIMER_STOP(key_name)`
    - `MEMFAULT_METRIC_ADD(key_name, amount)`

    These APIs can be used in place of the original APIs:

    - `memfault_metrics_heartbeat_set_signed(MEMFAULT_METRICS_KEY(key_name), signed_value)`
    - `memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(key_name), unsigned_value)`
    - `memfault_metrics_heartbeat_set_string(MEMFAULT_METRICS_KEY(key_name), value)`
    - `memfault_metrics_heartbeat_timer_start(MEMFAULT_METRICS_KEY(key_name))`
    - `memfault_metrics_heartbeat_timer_stop(MEMFAULT_METRICS_KEY(key_name))`
    - `memfault_metrics_heartbeat_add(MEMFAULT_METRICS_KEY(key_name), amount)`

    Saving some typing!

  - Add the ability to compute FreeRTOS task stack high watermarks when storing
    coredumps. This is useful only if the entire RAM (`.data` + `.bss`) cannot
    be saved in the coredump. The feature is opt-in with the config flag
    `#define MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE 1`.

  - Add a `heartbeat` command to the [core demo cli](components/demo). This
    behaves the same as the commands of the same name already present in the
    Zephyr + ESP-IDF port.

  - Add a `test_cassert` command to the core demo cli. This command executes a C
    stdlib `<assert.h>``assert(0)` call. For platforms that do not implement a
    `assert()` handler, a config flag `MEMFAULT_DEMO_DISABLE_CASSERT` can be
    defined to `0` to disable the command.

- ESP-IDF:

  - Add a new out-of-box metric, `wifi_ap_oui`, which will record the associated
    AP's Organizationally Unique Identifier (OUI) in the Memfault heartbeat.

- Zephyr:

  - Organize the Memfault Coredump Kconfig settings under a
    `Memfault Coredump Settings` submenu, for easier navigation when using
    graphical Kconfig frontends like menuconfig.

### :chart_with_upwards_trend: Improvements

- General:

  - Disable a warning emitted by the ARM C Compiler v5
    (`#188-D: enumerated type mixed with another type`) when initializing a
    structure in
    [`components/core/src/memfault_log.c`:314](https://github.com/memfault/memfault-firmware-sdk/blob/1.4.3/components/core/src/memfault_log.c#L313).

  - Improve the quality of Assert backtraces when using the ARM C Compiler v5.
    Certain frames in the assert call stack were missing link register
    inforation, due to compiler optimizations based on the `noreturn` and
    unreachable compiler hints. These hints have been removed for `armcc`, which
    should permit full stack unwinding for Assert coredumps generated from
    builds on that toolchain.

  - Perform an update of the timer when calling the
    `memfault_metrics_heartbeat_timer_read()` debug function. Fixes
    [#65](https://github.com/memfault/memfault-firmware-sdk/pull/65). Thanks to
    @LuskeyNoah for providing this fix!

- ESP-IDF:

  - Fix a missing piece enabling the "zero-config" integration (originally added
    in `1.4.0`)- the `memfault_platform_port.h` file was still incorrectly
    required. This is now fixed.

## [1.4.2] - 2023-11-02

### :chart_with_upwards_trend: Improvements

- General:

  - Improve the trace quality for asserts when using the IAR compiler and high
    optimization settings (`-Oh`)
  - Add demo CLI command to print the current values of heartbeat metrics. Try
    this out in your demo CLI implementation or with the built-in CLI with
    Zephyr!

## [1.4.1] - 2023-10-31

### :rocket: New Features

- ESP-IDF:

  - Add the following built-in heap allocation metrics by default. These can be
    disabled with the `CONFIG_MEMFAULT_ESP_HEAP_METRICS` Kconfig flag.

    - `heap_free_bytes`
    - `heap_largest_free_block_bytes`
    - `heap_allocated_blocks_count`
    - `heap_min_free_bytes`

### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Enable capturing [Memfault-style compact logs](https://mflt.io/compact-logs)
    on Zephyr systems. Note that this does not enable decoding
    [Zephyr "dictionary logs"](https://docs.zephyrproject.org/3.5.0/services/logging/index.html#dictionary-based-logging),
    but requires using the Memfault logging APIs directly (i.e.
    `MEMFAULT_LOG_INFO("...")` instead of `LOG_INF("...")`).

- General:

  - Add a `coredump_size` CLI command to the Zephyr, ESP-IDF, and demo CLI
    implementations. This command will print the computed size of the coredump
    and the available storage space. Can be used to tune coredump size.

  - Enable providing the Memfault HTTP Client with a custom
    `memfault_platform_get_device_info()` callback, for when the device is
    uploading data for a downstream device, with different device info.

  - When [compact logging](https://mflt.io/compact-logs) is enabled, route all
    `MEMFAULT_LOG_x()` statements through the compact serializer
    (`MEMFAULT_COMPACT_LOG_SAVE`). Previously, logs had to explicitly use the
    `MEMFAULT_COMPACT_LOG_SAVE` API to store in the compact form.

  - Capture C stdlib `assert.h` asserts, by implementing the correct assert
    hooks for Newlib/Picolibc and IAR libc's. This can be disabled with the
    Memfault platform config `MEMFAULT_ASSERT_CSTDLIB_HOOK_ENABLED`. This should
    improve the Trace quality for systems that are using the C stdlib
    `assert(x)` functions.

### :chart_with_upwards_trend: Improvements

## [1.4.0] - 2023-10-23

### :rocket: New Features

- ESP-IDF:

  - Implement support for a "zero-config" integration on ESP-IDF. This change
    makes the template + platform config files optional. See the
    [ESP32 Integration Guide](https://mflt.io/esp-tutorial) section on
    configuration files for details.

### :chart_with_upwards_trend: Improvements

- ESP-IDF:

  - Fix a build issue when overriding the default device-info implementation,
    `CONFIG_MEMFAULT_DEFAULT_GET_DEVICE_INFO=n`. This was a regression in
    **1.3.3**.

  - Add a new Kconfig flag, `MEMFAULT_COREDUMP_STORAGE_MAX_SIZE`, which can be
    used to set the Memfault SDK's built-in
    [ESP-IDF coredump storage implementation](https://github.com/memfault/memfault-firmware-sdk/blob/master/ports/esp_idf/memfault/common/memfault_platform_coredump.c)
    to artificially limit the maximum coredump storage size. This is useful for
    situations where the default `memfault_platform_coredump_get_regions()`
    function is still desirable, but the coredump maximum size needs to be
    limited (eg for bandwidth reasons).

  - Switch to using `ESP_DRAM_LOGE` when logging an error from Memfault's
    FreeRTOS task tracking when the number of tasks created on the system
    exceeds `MEMFAULT_PLATFORM_MAX_TRACKED_TASKS` (default of 16).

- General:

  - Remove the setting of the Memfault Firmware SDK version into a string metric
    on system boot. This value is now automatically extracted by the Memfault
    backend from the symbol file, and does not need to be set by devices.

## [1.3.5] - 2023-10-14

### :chart_with_upwards_trend: Improvements

- ESP-IDF:

  - Fix build errors when building the [ESP32 example app](examples/esp32)
    project on ESP-IDF versions earlier than v5. This was a regression in
    **1.3.4**.

  - Conditionally remove a redundant call to `esp_timer_init()` during the
    `memfault_boot()` sequence, which was causing a runtime error message. This
    call was originally required when `memfault_boot()` was called as part of
    ESP-IDF system init, which was disabled by default in **0.31.4** and
    deprecated in **0.40.0**. The redundant call was harmless but generated a
    nuisance error message.

- Fix another build error in the public unit tests caused by the `-fanalyzer`
  flag. The `-fanalyzer` static analyzer was generating a false positive on
  GCC 11. Updating the GCC version to 12 removes the false positive.

## [1.3.4] - 2023-10-12

### :chart_with_upwards_trend: Improvements

- ESP-IDF:

  - Add a missing dependency to the `memfault` component, `esp_https_ota`. It's
    only linked into the target project if the `memfault_esp_port_ota_update()`
    API is used. This was previously an implicit dependency from the `common`
    component dependencies, when building `memfault` into an ESP-IDF
    component-style project. This change fixes building for non-component-style
    ESP-IDF projects, where the default component dependencies might not be
    included.

  - Multiple changes to the
    [`examples/esp32`](examples/esp32/apps/memfault_demo_app) sample project:

    - Add a new `coredump_size` shell command, which prints out the maximum
      coredump size and the available coredump storage capacity.
    - Add new `settings_[get|set]` shell commands, and enable setting the LED
      brightness and blink interval to NVS. This is intended as a minor
      quality-of-life change for internal Memfault users of the example app.

- Fix a build error in the public unit tests, caused by the recent addition of
  the `-fanalyzer` flag to the unit test compilation options.

## 1.3.3 - Oct 10, 2023

### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Add a new Kconfig flag, `CONFIG_MEMFAULT_FAULT_HANDLER_RETURN`, which will
    call the normal `z_fatal_error` handler at the end of Memfault fault
    processing instead of rebooting the system. This is useful when user code
    needs to run within `k_sys_fatal_error_handler()` just prior to system
    shutdown. Thanks to @JordanYates for the patch! Fixes
    [#59](https://github.com/memfault/memfault-firmware-sdk/issues/59).

  - Add a timeout to the initial `send()` socket operation in
    `memfault_zephyr_port_http_upload_sdk_data()`, to abort the transfer if the
    socket is blocking for too long. That function will execute repeated
    `send()` calls to drain all the buffered Memfault data; this update only
    changes the initial call to check for a timeout, but otherwise will keep
    trying until the process completes, or a watchdog triggers. This is to
    balance the existing behavior, where a badly performing socket will still
    eventually push data through, but improves the case where the socket fails
    on the initial send (more common failure mode).

  - Remove a nuisance build warning generated when configured with
    `CONFIG_LOG_PRINTK=y && CONFIG_LOG_MODE_DEFERRED=y`. This impacts the
    usability of exporting base64-encoded chunks on the shell for testing
    (`mflt export` command), but is otherwise harmless.

- ESP-IDF:

  - Multiple changes to the
    [`examples/esp32`](examples/esp32/apps/memfault_demo_app) sample project:

    - Disable WiFi SoftAP by setting `CONFIG_ESP_WIFI_SOFTAP_SUPPORT=n`, since
      it's unused in the sample app. This saves about 40kB flash.
    - Permit setting the Memfault Project Key at runtime, with a new cli command
      `project_key`. The key is saved in Non-Volatile Storage on the ESP32
      board.

- General:

  - Enable using compact logs with the IAR build tools, by adding the needed
    `__no_alloc` attribute to the compact log symbols, to have the IAR linker
    set the `NO_LOAD` attribute correctly on the compact log output section.

### :boom: Breaking Changes

- ESP-IDF:

  - The [ESP-IDF port](ports/esp_idf/) now implements a default
    `memfault_get_device_info()` function, which uses the device MAC address for
    the Memfault Device Serial. When updating the Memfault SDK in an existing
    project, this implementation will cause a **linker error** due to duplicate
    definition. To disable the built-in definition, set
    `CONFIG_MEMFAULT_DEFAULT_GET_DEVICE_INFO=n`.

## 1.3.2 - Sept 26, 2023

### :chart_with_upwards_trend: Improvements

- Zephyr:

  - use `<cmsis_core.h>` instead of `<nmi.h>`. Thanks @kmeinhar for this change!
    (see [#64](https://github.com/memfault/memfault-firmware-sdk/pull/64))

- nRF Connect SDK:

  - Add missing Kconfig flags `CONFIG_FLASH_MAP=y` + `CONFIG_STREAM_FLASH=y` for
    the [`examples/nrf-connect-sdk/nrf5/`](examples/nrf-connect-sdk/nrf5/)
    example app, for compatibility with nRF Connect SDK v2.4.1+. This was
    required due to an
    [upstream Zephyr change](https://github.com/zephyrproject-rtos/zephyr/commit/1b4b979f8789af6087f877c0daad0a660c1b9b28).

- General:
  - Add support for Memfault Compact Logs for C++ source files (previously only
    supported in C source files). Compact logging can be enabled by setting
    `MEMFAULT_LOG_COMPACT_ENABLE=1` in `memfault_platform_config.h`. See
    [the docs](https://docs.memfault.com/docs/mcu/compact-logs/) for more
    details.
  - Fix a missing include of `<intrinsics.h>` required by the IAR compiler

## 1.3.1 - Sept 21, 2023

### :chart_with_upwards_trend: Improvements

- Zephyr:
  - Add a new Kconfig, `CONFIG_MEMFAULT_COREDUMP_COMPUTE_THREAD_STACK_USAGE`, to
    control whether thread stack usage is computed and collected on device
    during a coredump.

## 1.3.0 - Sept 20, 2023

### :chart_with_upwards_trend: Improvements

- General:
  - Remove an unused root certificate and adjust the order of certs to minimize
    additional TLS handshakes; DigiCert G2 is the most likely root cert so it is
    now the first one attempted in every place certificates are used. This
    update is trickled down from
    [DigiCert's pivot in March 2023](https://knowledge.digicert.com/generalinformation/digicert-root-and-intermediate-ca-certificate-updates-2023.html)
    to their G2 root cert as their default certificate.
- Zephyr:
  - Add support for the new location of the `nmi.h` header which
    [moved recently](https://github.com/zephyrproject-rtos/zephyr/pull/60031).

## 1.2.5 - Sept 18, 2023

### :chart_with_upwards_trend: Improvements

- Add MQTT transport to esp32 example app

## 1.2.4 - Sept 12, 2023

### :chart_with_upwards_trend: Improvements

- Fix a unit test failure introduced in 1.2.3. No on-target code was impacted.

## 1.2.3 - Sept 12, 2023

### :chart_with_upwards_trend: Improvements

- General:
  - Include the current MCU SDK version in the http client user agent header
- Zephyr:
  - Update include paths for CMSIS headers for upcoming Zephyr support. Thanks
    @gmarull for the patch!
- Modus Toolbox:
  - Allow a user application to override
    `memfault_metrics_heartbeat_collect_data`.

## 1.2.2 - Sept 5, 2023

### :chart_with_upwards_trend: Improvements

- ESP-IDF:
  - Correct the heartbeat metrics key definition include configuration to
    generate a consistent key index across the application. This regressed in
    version 1.0.1 of the SDK.

### Changes between Memfault SDK 1.2.0 and 1.2.1 - Sept 1, 2023

#### :chart_with_upwards_trend: Improvements

- ESP-IDF:
  - Add a `heartbeat` cli command to the ESP-IDF port that does the same thing
    as in Memfault's default demo console + Zephyr's console.
- Zephyr:
  - Add a new Kconfig option to the Zephyr port,
    `CONFIG_MEMFAULT_PLATFORM_EXTRA_CONFIG_FILE=y`, which causes
    `memfault_platform_config_extra.h` to be included in platform
    configurations. This can be utilized by a third party consumer of Zephyr to
    more easily extend the platform configurations set by Zephyr and avoid
    potentially losing a user's platform configurations set in
    `memfault_platform_config.h`, which
    [happened in the nRF Connect SDK recently](https://devzone.nordicsemi.com/f/nordic-q-a/103188/memfault-sdk-integration-missing-user-memfault_platform_config).

### Changes between Memfault SDK 1.1.3 and 1.2.0 - Aug 30, 2023

#### :chart_with_upwards_trend: Improvements

- ESP-IDF:
  - Eliminate several build warnings for our example app
- FreeRTOS:
  - Fix a build error in our QEMU example when building on macOS
- Modus Toolbox:
  - Update port to handle user override of SDK configs, metrics, trace_reasons,
    etc
  - Update port to fix errors when disabling built-in WiFi metrics
- nRF Connect SDK:
  - Enable mcuboot & NCS 1.4 with our example apps
- Silicon Labs:
  - Add a demo CLI component for the emblib port. Check out our
    [Simplicity Studio example app](https://github.com/memfault/simplicity-studio-example)
    for usage
  - Fix a build warning in emblib port flash storage (MSC) module
- Zephyr:
  - Fix a :bug: when building with LOG_MODE_DEFERRED that prevent log messages
    from flushing during a coredump
  - Fix a :bug: and warnings involving older Zephyr header paths. Resolves
    [#62](https://github.com/memfault/memfault-firmware-sdk/issues/62) and
    [#57](https://github.com/memfault/memfault-firmware-sdk/issues/57). Thanks
    @JordanYates and @YHusain1 for reporting these issues.

### Changes between Memfault SDK 1.1.2 and 1.1.3 - Aug 8, 2023

#### :chart_with_upwards_trend: Improvements

- Set Memfault SDK version in a string metric on device boot, for easier
  tracking of SDK versions in the Memfault UI
- Support using a different identifier for the GNU build id symbol (previously
  was fixed to `__start_gnu_build_id_start`). Use the
  `MEMFAULT_GNU_BUILD_ID_SYMBOL` define in `memfault_platform_config.h` to
  override the default. For Zephyr, the Kconfig option
  `CONFIG_MEMFAULT_GNU_BUILD_ID_SOURCE_BUILTIN` can be used to override the
  builtin section specifier + linker fragment for the GNU build ID. Thanks to
  @JordanYates for posting this change in
  [#60](https://github.com/memfault/memfault-firmware-sdk/pull/60) 🎉
- Improvements to the ARMv7-R exception handling when the supervisor processor
  mode is active

- Zephyr:
  - Add an optional mode to create and open the HTTP socket in separate function
    calls, if the user needs to set additional socket options before connecting.
    See
    [ports/zephyr/include/memfault/ports/zephyr/http.h](ports/zephyr/include/memfault/ports/zephyr/http.h)
    for details. Fixes
    [#52](https://github.com/memfault/memfault-firmware-sdk/issues/52)- thanks
    to @anicare-tero for posting this 🎉

### Changes between Memfault SDK 1.1.1 and 1.1.2 - July 11, 2023

#### :chart_with_upwards_trend: Improvements

- Improve compatibility in
  [reboot reason tracking](ports/emlib/rmu_reboot_tracking.c) and
  [watchdog implementation](ports/emlib/wdog_software_watchdog.c) on Silicon
  Labs Series 2 MCUs

- Zephyr:
  - Fix a build error when `CONFIG_MEMFAULT_LOGGING=n`, see
    [#56](https://github.com/memfault/memfault-firmware-sdk/issues/56). Thanks
    to @JordanYates for reporting this issue!
  - Fix a potential bug in the Memfault Log Backend when
    `CONFIG_LOG_MODE_IMMEDIATE=y` when flushing of fault logs during a crash

### Changes between Memfault SDK 1.1.0 and 1.1.1 - June 30, 2023

#### :chart_with_upwards_trend: Improvements

- Reduce the amount of error logs emitted by the MbedTLS port HTTP client while
  polling for session established. This regressed in SDK version 1.1.0.

### Changes between Memfault SDK 1.0.1 and 1.1.0 - June 29, 2023

#### :chart_with_upwards_trend: Improvements

- New built-in metrics 🎉 !

  - FreeRTOS Idle Task runtime percentage metric: `idle_task_run_time_percent`.
    This is automatically enabled for FreeRTOS builds with the correct tracing
    options enabled, see
    [ports/include/memfault/ports/freertos/metrics.h](ports/include/memfault/ports/freertos/metrics.h)
    for details on how to enable or disable this metric.
  - MbedTLS metrics for maximum and current memory used: `mbedtls_mem_max_bytes`
    and `mbedtls_mem_used_bytes`. These are automatically enabled for ESP-IDF
    projects, see
    [ports/mbedtls/memfault_mbedtls_metrics.c](ports/mbedtls/memfault_mbedtls_metrics.c)
    for usage details
  - Renamed the built-in LwIP metrics added in SDK version 1.0.1 from
    `Tcp_Drop_Count`/`Tcp_Rx_Count`/`Tcp_Tx_Count`,
    `Udp_Drop_Count`/`Udp_Rx_Count`/`Udp_Tx_Count` to be all lowercase
    `tcp_drop_count`/`tcp_rx_count`/`tcp_tx_count`,
    `udp_drop_count`/`udp_rx_count`/`udp_tx_count`
  - Add the following automatically enabled WiFi performance metrics to the
    ESP-IDF port:
    - `wifi_connected_time_ms`
    - `wifi_disconnect_count`
    - `wifi_sta_min_rssi`

- Fix a bug in the [mbedtls port](ports/mbedtls/) causing an infinite loop under
  certain error conditions on TLS handshake

- Zephyr:

  - Improve log flushing in the Memfault log backend during fault when deferred
    logging is enabled. This ensures the latest log statements are included in
    the coredump log data, when the `CONFIG_MEMFAULT_LOGGING_ENABLE=y`

- ESP-IDF:

  - `ESP_ERROR_CHECK()` assert coredumps will now correctly show as assert in
    the Memfault coredump analysis view, instead of "Hard Fault"

### Changes between Memfault SDK 1.0.1 and 1.0.0 - June 9, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Improve the quality of Zephyr stack usage detection when capturing less than
    the full stack contents for each thread. This feature was originally
    released in SDK `0.43.0`.

- ESP-IDF:

  - Add Memfault Metrics tracking LwIP runtime statistics (packet counts). A new
    Kconfig flag `CONFIG_MEMFAULT_LWIP_METRICS` controls this feature, which is
    enabled by default. The LwIP metrics helper is available for non-ESP-IDF
    projects using LwIP, see
    [`ports/lwip/memfault_lwip_metrics.c`](ports/lwip/memfault_lwip_metrics.c)
    for details

### Changes between Memfault SDK 1.0.0 and 0.43.3 - June 1, 2023

🎉🎉🎉

The Memfault Firmware SDK is now version `1.0.0` as of this release! Note that
this is just a procedural change, there are no breaking backwards-incompatible
changes in this release. We forgot to update our major version back in 2019, but
better late than never 😅! Hopefully the remaining 281,474,976,710,656 versions
are enough 🤞.

🎉🎉🎉

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Add a new built-in metric `FileSystem_BytesFree` for tracking VFS bytes
    free. This is enabled automatically when `CONFIG_FILE_SYSTEM=y`. Use the
    Kconfig option `MEMFAULT_FS_BYTES_FREE_VFS_PATH` to set the VFS mount point
    to monitor utilization (default value is `/lfs1`). The Kconfig option
    `MEMFAULT_FS_BYTES_FREE_METRIC` can be used to disable the metric.
  - Update a few spots in the Zephyr demo CLI to use `shell_print` instead of
    `MEMFAULT_LOG` for command usage errors

### Changes between Memfault SDK 0.43.3 and 0.43.2 - May 22, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Add more granular Kconfig settings to control what's collected in
    [`memfault_zephyr_coredump_get_regions()`](ports/zephyr/common/memfault_platform_coredump_regions.c).
    Default settings are identical to before this change.

    - `CONFIG_MEMFAULT_COREDUMP_COLLECT_STACK_REGIONS`
    - `CONFIG_MEMFAULT_COREDUMP_COLLECT_KERNEL_REGION`
    - `CONFIG_MEMFAULT_COREDUMP_COLLECT_TASKS_REGIONS`

  - Fix a build error when an application is configured without
    `CONFIG_HEAP_MEM_POOL_SIZE=y` (i.e. no `kmalloc` in use).

- Fix a build error when building the
  [nRF9160 example](examples/nrf-connect-sdk/nrf9160) on nRF-Connect SDK v2.3.0.

- ESP-IDF:

  - Add support for upcoming v5.1 release of the ESP-IDF SDK
  - Add support in the [ESP32 example app](examples/esp32) for the ESP32-C6 chip

### Changes between Memfault SDK 0.43.2 and 0.43.1 - May 3, 2023

#### :chart_with_upwards_trend: Improvements

- Fix a bug 🐛 where metrics accumulated with `memfault_metrics_heartbeat_add()`
  would no longer be included in the serialized heartbeat data. This regression
  occurred in `0.42.0`.

### Changes between Memfault SDK 0.43.1 and 0.43.0 - April 26, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - The `z_NmiHandlerSet` function is renamed for the upcoming Zephyr 3.4.
    Support the new name. Fixes
    [#49](https://github.com/memfault/memfault-firmware-sdk/issues/49). Thanks
    @mbolivar-nordic for filing this issue!

### Changes between Memfault SDK 0.43.0 and 0.42.1 - April 18, 2023

#### :rocket: New Features

- Add coredump support for Cortex-R chips (ARMv7-R)

#### :chart_with_upwards_trend: Improvements

- Add a QEMU-based FreeRTOS example project, find it under
  [examples/freertos](examples/freertos)
- Switch `printf` function attribute to use `__printf__`, to avoid collision
  with user code that redefines the `printf` token

- Zephyr:

  - Compute thread stack high watermark values on-target when capturing a
    coredump. This enables showing high watermark information in the Memfault
    coredump analysis view without capturing the full stack for a thread

### Changes between Memfault SDK 0.42.1 and 0.42.0 - April 4, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Remove `LEGACY_CONFIG_PATH` Kconfig selection, now that the `zephyr.h`
    header is no longer used as of Memfault SDK `0.42.0`. This option no longer
    exists after Zephyr v3.3.0. Fixes
    [#48](https://github.com/memfault/memfault-firmware-sdk/issues/48)

- Minor changes to the [ESP8266 port](ports/esp8266_sdk) to improve
  out-of-the-box compilation
- Add new functionality to output buffered log data via
  `memfault_log_export_logs()`. See the
  [`log.h` header](components/include/memfault/core/log.h) for detailed usage.

### Changes between Memfault SDK 0.42.0 and 0.41.2 - Mar 22, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:

  - Add option to capture full thread stacks for classifying stack overflows and
    determining stack high watermarks. This feature is enabled by setting
    `CONFIG_MEMFAULT_COREDUMP_FULL_THREAD_STACKS=y`
  - Remove usage of the `zephyr.h` header in preparation for Zephyr v3.4.0.
    Thanks to @jfischer-no for the patch!

- `memfault_gdb.py`:
  - Add support for exporting data from GCC 12 compiled symbol files
  - Add arguments to override device serial ID, software type, software version,
    and hardware revision

#### :boom: Breaking Changes

- Metrics:
  - Integer type metrics (signed/unsigned) will reset to NULL when not set
    during a heartbeat interval. This NULL value will be discarded by Memfault
    when received. The previous behavior was to reset to 0 which makes
    discarding values difficult as 0 is a valid value for these types. For more
    info please see the
    [Metrics](https://docs.memfault.com/docs/mcu/metrics-api#setting-metric-values)
    docs.

### Changes between Memfault SDK 0.41.2 and SDK 0.41.1 - Mar 10, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr / nRF-Connect SDK:
  - Improve compatibility with Zephyr pre-3.0 deferred logging, when using the
    Memfault backend
  - Add an option to the
    [examples/nrf-connect-sdk/nrf5](examples/nrf-connect-sdk/nrf5) to enable
    capturing all of RAM in a coredump.

### Changes between Memfault SDK 0.41.1 and SDK 0.41.0 - Mar 1, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr / nRF-Connect SDK:

  - Fix a bug 🐛 in the Memfault Logging backend, that causes coredump saving to
    fail when building with Zephyr versions before 3.0 (nRF-Connect SDK < 2.0).
    This regression was introduced in Memfault Firmware SDK **0.33.3**.

- ESP-IDF:
  - Updated the [ESP32 example app](examples/esp32) for the ESP32-C3 on ESP-IDF
    v5.0.1 to use size optimizations, to more closely mirror real world
    environments and codesize.
  - Fix a compilation error in the [ESP32 example app](examples/esp32) when
    targeting ESP-IDF >=4.4,<4.4.3 . This regression was introduced in Memfault
    Firmware SDK **0.39.1**.

### Changes between Memfault SDK 0.41.0 and SDK 0.40.0 - Feb 22, 2023

#### :rocket: New Features

- ESP-IDF:
  - Added coredump support for the ESP32-C3 (RISC-V) chip. Thank you to @jlubawy
    for your work on this in
    [#42](https://github.com/memfault/memfault-firmware-sdk/pull/42) 🎉!

### Changes between Memfault SDK 0.40.0 and SDK 0.39.1 - Feb 15, 2023

#### :bomb: Breaking Changes

- ESP-IDF:
  - The Kconfig `CONFIG_MEMFAULT_AUTOMATIC_INIT` has been deprecated and is no
    longer supported. Users of this Kconfig should refactor their application to
    call `memfault_boot` during initialization. Use of this Kconfig now results
    in a build error. For more information please see
    https://docs.memfault.com/docs/mcu/esp32-guide#initializing-memfault

### Changes between Memfault SDK 0.39.1 and SDK 0.39.0 - Feb 3, 2023

#### :rocket: New Features

- **Experimental**
  - CMSIS-Pack support
  - Out Of Memory reboot reason added

#### :chart_with_upwards_trend: Improvements

- ESP-IDF:
  - The default implementation of `memfault_platform_coredump_get_regions` is
    changed to collect the current active stack, .bss, .data, and .heap regions.
    Additionally if you are using ESP-IDF >= 4.4.0, the SDK will prioritize
    collecting FreeRTOS regions containing task TCB and stack data.
  - Assert coredumps are now labeled with the Assert reason

### Changes between Memfault SDK 0.39.0 and SDK 0.38.0 - Feb 3, 2023

#### :boom: Breaking Changes

- Breaking changes to the
  [`memfault_freertos_get_task_regions()`](ports/freertos/src/memfault_freertos_ram_regions.c)
  function, which can be used to capture FreeRTOS tasks when coredumps are sized
  smaller than all available RAM. The function will now, by default, capture a
  truncated copy of each FreeRTOS TCB, instead of the complete structure. This
  makes better use of coredump storage space; the TCB structures can be very
  large (>1kB), but Memfault only needs the first few fields for coredump
  decoding. The configuration flag `MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE` (see
  [`default_config.h`](components/include/memfault/default_config.h)) can be set
  to `0` in `memfault_platform_config.h` to return to the previous behavior.

### Changes between Memfault SDK 0.38.0 and SDK 0.37.2 - Feb 1, 2023

#### :rocket: New Features

- Enable coredumps on the ESP32-S2 and ESP32-S3 chips.

### Changes between Memfault SDK 0.37.2 and SDK 0.37.1 - Jan 31, 2023

#### :chart_with_upwards_trend: Improvements

- Zephyr:
  - Support building with `CONFIG_POSIX_API=y` in the Zephyr port HTTP client
- ESP-IDF:
  - Reduce the spamminess of the esp32 example app logging
- Update [`scripts/memfault_gdb.py`](scripts/memfault_gdb.py):
  - When explicitly listing a region to insert into the coredump via
    `memfault coredump --region x y`, now support parseable GDB expressions for
    the range arguments instead of requiring integer values. Thanks to @alvarop
    for this patch
    [#43](https://github.com/memfault/memfault-firmware-sdk/pull/43) !
  - Use the `info all-registers` command when dumping registers, instead of the
    deprecated `info registers all` command, which works better on certain
    arch/monitor setups. Thanks to @alvarop for this patch
    [#44](https://github.com/memfault/memfault-firmware-sdk/pull/44) !

### Changes between Memfault SDK 0.37.1 and SDK 0.37.0 - Jan 17, 2023

#### :chart_with_upwards_trend: Improvements

- FreeRTOS
  - Add support for collecting truncated TCBs. This is particularly useful with
    limited coredump space. Enable by defining
    `MEMFAULT_PLATFORM_FREERTOS_TCB_SIZE` in `memfault_platform_config.h".
- ESP-IDF
  - Add support for esp32s2/s3 platforms to the esp32 example application

#### :house: Internal

- Fixup some documentation typos/errors
- Add support for Python 3.10

### Changes between Memfault SDK 0.37.0 and SDK 0.36.1 - Dec 16, 2022

#### :chart_with_upwards_trend: Improvements

- Built-in Metrics

  - Add `MemfaultSdkMetric_UnexpectedRebootDidOccur` metric. This metric uses
    the platform's reboot register and any reasons by the SDK function
    `memfault_reboot_tracking_mark_reset_imminent` to classify a reboot. When
    reboot tracking determines a reboot is unexpected, this metric is set to 1.
    Otherwise this metric is 0.

- [ModusToolbox:tm: Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)

  - Add log capture during coredump to port

- Demo CLI
  - Add `mflt test loadaddr` command. This comamnd is used to test specific
    faults due to protected regions

#### :boom: Breaking Changes

- Built-in Metrics
  - The built-in metric, `MemfaultSdkMetric_UnexpectedRebootDidOccur`,
    classifies all reboot reasons greater than or equal to
    `kMfltRebootReason_UnknownError` **or** equal to `kMfltRebootReason_Unknown`
    as "unexpected reboots". It is recommended to ensure your platform's
    implementation of `memfault_reboot_reason_get` classifies the reboot
    register values as accurately and precisely as possible to avoid incorrect
    metric values.

### Changes between Memfault SDK 0.36.1 and SDK 0.36.0 - Dec 9, 2022

#### :chart_with_upwards_trend: Improvements

- ESP-IDF:
  - Fix a bug 🐛 in the [ESP32 example app](examples/esp32), where wifi join
    fails when using versions of ESP-IDF prior to 5.0

### Changes between Memfault SDK 0.36.0 and SDK 0.35.0 - Dec 6, 2022

#### :chart_with_upwards_trend: Improvements

- ESP-IDF:

  - Add support for
    [just-released ESP-IDF v5](https://github.com/espressif/esp-idf/releases/tag/v5.0)
    🎉! Thanks to @jlubawy and the patch supplied in #39 for this, very much
    appreciated!
  - Add an auto-OTA (and auto-WiFi-join) feature to the
    [ESP32 example app](examples/esp32)- enabled by default but can be disabled
    with Kconfig

- The [Heap Stats tracing component](https://mflt.io/mcu-heap-stats) has been
  revamped to make more efficient usage of the bookeeping structure. Usage
  should be the same as before, but now should provide more data without
  significantly expanding the memory utilization.

### Changes between Memfault SDK 0.35.0 and SDK 0.34.2 - Nov 22, 2022

#### :rocket: New Features

- **Experimental** Custom Data Recording API
  - Allows sending custom data collected over the course of a recording period

#### :chart_with_upwards_trend: Improvements

- Zephyr:
  - Modify heap stats to only collect info during allocations/deallocations from
    threads
- ESP-IDF:
  - ESP32 reboot tracking into RTC noinit
- nRF5 SDK:
  - NRF5 coredump regions -Wunused-macros, fixes warning for unused macros

#### :house: Internal

- Experiment: pytest as fw test frontend
- README: Add additional details on port integration

### Changes between Memfault SDK 0.34.2 and SDK 0.34.1 - Nov 8, 2022

#### :chart_with_upwards_trend: Improvements

- [ModusToolbox:tm: Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
  - Updates SDK for compatibility with MTB 3.0

### Changes between Memfault SDK 0.34.1 and SDK 0.34.0 - Nov 7, 2022

#### :chart_with_upwards_trend: Improvements

- nRF-Connect:
  - Updates for Zephyr upmerge 2022.11.03 (see #35 + #36)
  - Fix watchdog test (`mflt test hang`) in
    [`examples/nrf-connect-sdk/nrf5/`](examples/nrf-connect-sdk/nrf5/)
- Zephyr:
  - Set `CONFIG_QEMU_ICOUNT=n` in
    [`examples/zephyr/qemu/`](examples/zephyr/qemu/), which fixes the emulated
    target execution speed
  - Add heap free and stack usage Metrics to
    [`examples/zephyr/qemu/`](examples/zephyr/qemu/)
- Update the `memfault_demo_cli_cmd_assert()` test command to take a single arg,
  which is used in `MEMFAULT_ASSERT_RECORD()`. This enables testing that assert
  variant from the CLI.

### Changes between Memfault SDK 0.34.0 and SDK 0.33.5 - Nov 1, 2022

#### :chart_with_upwards_trend: Improvements

- Misc ESP32 [port](ports/esp_idf) &
  [example app](examples/esp32/apps/memfault_demo_app) improvements
  - Added diagnostic print line containing Build Id at boot up
  - Improved messaging displayed when using `memfault_ota_check` test command
  - Example app now prints device info on bootup
  - Fix an issue where incremental build (`idf.py build && idf.py build`) would
    report a nuisance failure.
  - Flatten + simplify the directory structure of the QEMU based example project
- A new [`ports/mbedtls`](ports/mbedtls) is available, which implements a basic
  Mbed TLS client for performing Memfault data upload.
- Zephyr: Collect sysheap stats using the
  [Memfault Heap Tracking](https://mflt.io/mcu-heap-stats) component. This is
  configured with the `CONFIG_MEMFAULT_HEAP_STATS` Kconfig option (enabled by
  default), and will track allocations done with `k_malloc()`.
- Fix an enum-mismatch warning in `memfault_metrics.c` when using the ARMCC v5
  compiler.

#### :boom: Breaking Changes

- If you are using the ESP32 HTTP Client, the Memfault Project Key is now
  configured directly via the
  [ESP32 Project Configuration System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html).
  You need to do the following:
  1. Remove the `g_mflt_http_client_config` in your platform port
  2. Add `CONFIG_MEMFAULT_PROJECT_KEY="YOUR_PROJECT_KEY"` to your projects
     `sdkconfig.defaults`

### Changes between Memfault SDK 0.33.4 and SDK 0.33.5 - Oct 19, 2022

#### :chart_with_upwards_trend: Improvements

- nRF-Connect: Update the nRF9160 example application,
  `examples/nrf-connect-sdk/nrf9160`, to build and run correctly with
  nRF-Connect SDK v2.1.0
- Zephyr: Add an example Zephyr application targeting the QEMU board
- ESP-IDF:
  - Add a configuration option for setting the ESP-IDF HTTP client timeout value
  - Fix compilation for the ESP32-S3. _Note: coredumps are currently only
    supported on the ESP32, not the ESP32-S2, -S3, or -C3. This change only
    fixes compiling for the -S3 platform_
  - Add support for ESP-IDF v4.2.3

#### :house: Internal

- Support building the unit tests with GCC 12
- Miscellaneous fixes to unit test infrastructure to better support building in
  Mac OSX

### Changes between Memfault SDK 0.33.3 and SDK 0.33.4 - Sept 15, 2022

#### :chart_with_upwards_trend: Improvements

- Zephyr port updates:
  - Handle thread abort in the task stack capture hook. Previous to this change,
    aborted tasks would remain on the captured task list, and restarting the
    task would create a duplicate entry.

### Changes between Memfault SDK 0.33.2 and SDK 0.33.3 - Sept 14, 2022

#### :chart_with_upwards_trend: Improvements

- Zephyr port updates:
  - Add a call to `LOG_PANIC()` before running the Memfault fault handler, to
    flush any deferred logs before the reboot

### Changes between Memfault SDK 0.33.1 and SDK 0.33.2 - Sept 7, 2022

#### :chart_with_upwards_trend: Improvements

- Zephyr port updates:
  - fix a few minor nuisance build warnings on niche Zephyr configurations
  - enable `LOG_OUTPUT` when `MEMFAULT_LOGGING_ENABLE` is enabled- this fixes a
    build error if all other log backends are disabled. thanks to @balaji-nordic
    for this fix! closes #33
- Add a debug cli test command to the nRF-Connect SDK port for printing the OTA
  url

### Changes between Memfault SDK 0.33.0 and SDK 0.33.1 - Aug 26, 2022

#### :chart_with_upwards_trend: Improvements

- Fix a :bug: in the heap stats component (#32), thanks @christophgreen for
  reporting it!
- Zephyr port updates:
  - add support for the newly namespaced Zephyr include path in upcoming Zephyr
    v3.2 (`#include <zephyr.h>` → `#include <zephyr/zephyr.h>`). The includes
    were moved
    [prior to v3.1](https://github.com/zephyrproject-rtos/zephyr/commit/53ef68d4598b2f9005c5da3fc0b860ca1999d350)
    of Zephyr, but v3.2
    [changes the backwards compatibility support to opt-in](https://github.com/zephyrproject-rtos/zephyr/commit/1ec0c6f5308937dc8e77acc2567d6f53cdd7a74e).
    The Memfault SDK is now updated to support both.
  - fix Zephyr Memfault log capture to have the correct prefix in the decoded
    output when using LOG2 - previously all log lines regardless of level would
    have an `E` prefix (regression introduced in Memfault SDK version 0.32.0)
  - fix Zephyr Memfault log capture when in `CONFIG_LOG_MODE_IMMEDIATE` and
    using LOG2 to capture the full log line instead of each logged character as
    a separate line.

#### :house: Internal

- Zephyr port folder for `v2.x` migrated to `common`, now that Zephyr v1.14
  support has been removed (done in v0.32.0 of the Memfault SDK)
- Update README's for the example projects to match the new demo shell command
  structure (`crash 1` → `test_hardfault`, etc).
- Tidy up nrf9160 example app Kconfig setup
- Fix parallel unit test invocation

### Changes between Memfault SDK 0.33.0 and SDK 0.32.2 - Aug 18, 2022

#### :chart_with_upwards_trend: Improvements

- Extend [memfault demo shell](components/demo/src/memfault_demo_shell.c) to
  support terminals that only emit CR for line endings
- nRF5 SDK Updates:
  - Added a [software watchdog](https://mflt.io/root-cause-watchdogs) reference
    port for nRF5 SDK which makes use of the RTC Peripheral. See
    [ports/nrf5_sdk/software_watchdog.c](ports/nrf5_sdk/software_watchdog.c) for
    more details.
  - Updated [nRF5 example app](examples/nrf5/apps/memfault_demo_app/) to make
    use of hardware and new software watchdog port.
- Zephyr Port Updates:
  - Added Kconfig option to fallback to using `printk` by default when no
    logging is enabled. This can be disabled by setting
    `CONFIG_MEMFAULT_PLATFORM_LOG_FALLBACK_TO_PRINTK=n`.
- nRF Connect SDK Updates:
  - Fixed a :bug: which could result in download errors when using
    [Memfault nRF Connect SDK FOTA client](ports/nrf-connect-sdk/zephyr/include/memfault/nrfconnect_port/fota.h)
    and enabled client in example application by default.
  - Added new example application for trying Memfault with nRF53 & nRF52 based
    development kits. See
    [examples/nrf-connect-sdk/nrf5](examples/nrf-connect-sdk/nrf5) for more
    details.

### Changes between Memfault SDK 0.32.2 and SDK 0.32.1 - Aug 16, 2022

#### :chart_with_upwards_trend: Improvements

- Zephyr port: added a fix for upcoming Zephyr 3.2 compatibility, thanks
  @nordicjm for the fix!

### Changes between Memfault SDK 0.32.1 and SDK 0.32.0 - Aug 8, 2022

#### :house: Internal

- Added default config header for PSoC 6 port
  [ports/cypress/psoc6/psoc6_default_config.h](ports/cypress/psoc6/psoc6_default_config.h)
  so user doesn't have to create it

### Changes between Memfault SDK 0.32.0 and SDK 0.31.5 - Aug 8, 2022

#### :chart_with_upwards_trend: Improvements

- [ModusToolbox:tm: Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
  port updates
  - Added heartbeat metrics for heap and Wi-Fi performance tracking when using
    the default port for
    [CAT1A (PSoC:tm: 6)](https://github.com/Infineon/mtb-pdl-cat1). See
    [ports/cypress/psoc6/memfault_platform_core.c](ports/cypress/psoc6/memfault_platform_core.c)
    for more details
  - Fixed reboot reason reported when PSoC 6 is fully reset to report "Power On
    Reset" instead of "Unknown"
- Zephyr port updates
  - Memfault logs (eg `MEMFAULT_LOG_DEBUG()` etc) are now routed to the Zephyr
    logging infrastructure. The typical set of Kconfig options for Memfault logs
    are available (`CONFIG_MEMFAULT_LOG_LEVEL_WRN` etc). See details in
    "Breaking Changes" below for enabling logs in your project.
  - Added a new Kconfig option, `MEMFAULT_ZEPHYR_FATAL_HANDLER`, which can be
    used to disable the Zephyr fault handler print facilities.
  - Streamline support for nRF-Connect SDK based applications that don't need
    the Memfault root certificates (eg nRF53 or nRF52 devices), via a new
    Kconfig option `MEMFAULT_ROOT_CERT_STORAGE`, to avoid a nuisance build error

#### :boom: Breaking Changes

- Users will no longer see internal Memfault log output by default, but will
  have to enable it explicitly to see the output:

  ```ini
  # enable LOG
  CONFIG_LOG=y
  # not required- enabling the Memfault logging component enables including the
  # log buffer in coredumps
  CONFIG_MEMFAULT_LOGGING_ENABLE=y

  # if on pre-v3.1.0 zephyr, you can choose either the default LOG v1
  # implementation, or select a LOG2 mode to enable LOG2. on zephyr 3.1.0+, LOG
  # v1 is removed and LOG v2 is now the only log implementation
  # CONFIG_LOG2_MODE_DEFERRED=y

  # make sure to select a log backend to see the output
  CONFIG_LOG_BACKEND_UART=y
  ```

  The log statements affected by this change are likely only the internal
  Memfault SDK logs (`MEMFAULT_LOG_DEBUG()` etc), unless those macros are used
  in the user application.

- Removed support for Zephyr LTS release 1.14 as it was superseded by
  [LTS V2 almost a year ago now](https://www.zephyrproject.org/zephyr-lts-v2-release/).
  A project using this release of Zephyr must target a memfault-firmware-sdk
  release less than 0.32.0.

#### :house: Internal

- More logically grouped Kconfig settings in Zephyr example app's
  [prj.conf](examples/zephyr/apps/memfault_demo_app/prj.conf)
- Fixed a few typos in particle port documentation
- Simplified compilation steps for the
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app)
  when compiling with older releases of the nRF Connect SDK and refreshed the
  example to target the v2.0.2 release by default
- Updated default demo CLI commands to better align with
  [our suggested integration test commands](https://mflt.io/mcu-test-commands).
  The default set now looks like this:

  ```bash
  mflt> help
  clear_core: Clear an existing coredump
  drain_chunks: Flushes queued Memfault data. To upload data see https://mflt.io/posting-chunks-with-gdb
  export: Export base64-encoded chunks. To upload data see https://mflt.io/chunk-data-export
  get_core: Get coredump info
  get_device_info: Get device info
  test_assert: Trigger memfault assert
  test_busfault: Trigger a busfault
  test_hardfault: Trigger a hardfault
  test_memmanage: Trigger a memory management fault
  test_usagefault: Trigger a usage fault
  test_log: Writes test logs to log buffer
  test_log_capture: Trigger capture of current log buffer contents
  test_reboot: Force system reset and track it with a trace event
  test_trace: Capture an example trace event
  help: Lists all commands
  ```

### Changes between Memfault SDK 0.31.5 and SDK 0.31.4 - July 22, 2022

#### :chart_with_upwards_trend: Improvements

- Zephyr port: enable proper backtraces for Zephyr `__ASSERT()` macro on
  aarch32/cortex_m. Prior to this fix, crashes from `__ASSERT()` triggering
  would show an incorrect PC/LR for the active thread.

- Support for pre-release nRF Connect SDK v2.0.99 and Zephyr > v3.1:
  - Upcoming nRF Connect SDK and Zephyr releases removed logging v1. Add build
    support for these changes, and keep backwards compatibility for previous nRF
    Connect SDK/Zephyr releases
  - Correct an issue in the Memfault logging v2 backend, when the system was
    invoked from an ISR context. This could happen due to a recent change, in
    Memfault SDK v0.31.1, where the Zephyr fatal informational logs were output
    from `memfault_platform_reboot()` by default. It did not impact the
    collected backtrace, but it would show a nuisance `__ASSERT()` in the
    console output, if `CONFIG_ASSERT=y`.

#### :house: Internal

- Fix a compilation issue in the Dialog example app from the removal of
  `memfault_demo_cli_cmd_print_chunk()` in Memfault SDK release v0.31.4.

### Changes between Memfault SDK 0.31.4 and SDK 0.31.3 - July 19, 2022

#### :chart_with_upwards_trend: Improvements

- ESP32 port: add new Kconfig option, `CONFIG_MEMFAULT_AUTOMATIC_INIT`, that can
  be explicitly set to `n` to skip automatically initializing the Memfault SDK
  on boot. This can be useful if Memfault SDK initialization needs to be
  deferred to application start.

- Zephyr port: add Kconfig options,
  `CONFIG_MEMFAULT_INIT_PRIORITY`/`CONFIG_MEMFAULT_INIT_LEVEL_POST_KERNEL` for
  controlling the Memfault SDK initialization level and priority. This can be
  useful when needing Memfault to initialize earlier in the system startup
  sequence, for example for diagnosing crashes in an early driver
  initialization.

- Partial support, still in progress, for NRF Connect SDK + Zephyr v3.1:
  - Remove reference to the now-removed Kconfig symbol,
    `NET_SOCKETS_OFFLOAD_TLS` to enable building without warnings. **NOTE:** if
    mbedtls is enabled (`CONFIG_MBEDTLS=y`), but is _not_ being used for HTTP
    transfers (eg, mbedtls is used for security functions, but the device does
    not use HTTP for transferring data), it may be necessary to explicitly set
    `CONFIG_MEMFAULT_HTTP_USES_MBEDTLS=n`.

#### :house: Internal

- Zephyr port: remove an unused header file,
  `ports/zephyr/common/memfault_zephyr_http.h`

- Remove `memfault_demo_cli_cmd_print_chunk()` demo function.
  `memfault_data_export_dump_chunks()` can be used instead, which is intended to
  be used with the "Chunks Debug" UI in the Memfault web application- see
  [here](https://mflt.io/chunk-data-export) for more details

### Changes between Memfault SDK 0.31.3 and SDK 0.31.2 - July 8, 2022

#### :chart_with_upwards_trend: Improvements

- Support Zephyr v3.1+ by conditionally compiling out Logger v1 code, thanks to
  @tejlmand for the patch!

### Changes between Memfault SDK 0.31.2 and SDK 0.31.1 - June 24, 2022

#### :chart_with_upwards_trend: Improvements

- Fixed a :bug: in the
  [Zephyr port HTTP implementation](ports/zephyr/common/memfault_platform_http.c),
  where a socket file descriptor was leaked. This caused every HTTP operation
  after the first to fail on Zephyr platforms. Thanks to @rerickson1 for the
  fix!
- Added an update to improve the quality of stack traces when using
  `MEMFAULT_ASSERT` with the TI ARM Clang Compiler

### Changes between Memfault SDK 0.31.1 and SDK 0.31.0 - June 16, 2022

#### :chart_with_upwards_trend: Improvements

- Enable the Zephyr fault handler (including console fault prints) after
  Memfault handler runs. Can be disabled by implementing
  `memfault_platform_reboot()`. See details in
  [ports/zephyr/include/memfault/ports/zephyr/coredump.h](ports/zephyr/include/memfault/ports/zephyr/coredump.h)

#### :house: Internal

- Fixed compiler error in
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app)
  when compiling with the nRF Connect SDK v2.0.0 release

### Changes between Memfault SDK 0.31.0 and SDK 0.30.5 - June 6, 2022

#### :chart_with_upwards_trend: Improvements

- Added reference port for
  [CAT1A (PSoC:tm: 6)](https://github.com/Infineon/mtb-pdl-cat1) based MCUs
  using the
  [ModusToolbox:tm: Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
  stack. For more details see [ports/cypress/psoc6](ports/cypress/psoc6)
  directory.
- Added a convenience utility function for posting chunks using the Memfault
  http client. See
  [`memfault_http_client_post_chunk`](components/include/memfault/http/http_client.h#L101)
  for more details!

#### :house: Internal

- Fixed compiler error in
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app)
  when compiling with the nRF Connect SDK 1.8 release

### Changes between Memfault SDK 0.30.5 and SDK 0.30.4 - May 24, 2022

#### :rocket: New Features

- ESP-IDF: add Memfault Compact Log example integration to the
  [`examples/esp32`](examples/esp32) project

#### :chart_with_upwards_trend: Improvements

- ESP-IDF: Fix backtraces when using ESP-IDF v4.4+
- nRF-Connect SDK: enable the Kconfig flag `MEMFAULT_NRF_CONNECT_SDK` by default
  when targeting the nrf52 + nrf53 series SOCs (previously only enabled by
  default for nrf91 series)

#### :house: Internal

Added clarifications around licensing in ports and examples folders. See
[README](README.md) for more details.

### Changes between Memfault SDK 0.30.4 and SDK 0.30.3 - May 4, 2022

#### :chart_with_upwards_trend: Improvements

- minor updates to [`scripts/eclipse_patch.py`](scripts/eclipse_patch.py) to
  support NXP's MCUXpresso IDE

### Changes between Memfault SDK 0.30.3 and SDK 0.30.2 - April 25, 2022

#### :chart_with_upwards_trend: Improvements

- Particle's Device OS port improvements:
  - A user initiated reboot will now be recorded as a User Shutdown instead of a
    Low Power reset
  - A custom hardware_version can now be specified using the `hardware_version`
    argument when initializing the Memfault library
  - Default hardware version now uses the `PLATFORM_NAME` macro instead of
    `PRODUCT_SERIES` macro
- Zephyr port improvements
  - Exposed lower level APIs to Memfault's HTTP post implementation to allow
    easier custom handling. See
    [`ports/zephyr/include/memfault/ports/zephyr/http.h`](ports/zephyr/include/memfault/ports/zephyr/http.h)
    for more details

#### :house: Internal

- Misc README documentation improvements

### Changes between Memfault SDK 0.30.2 and SDK 0.30.1 - April 12, 2022

- Fix a build regression on nRF Connect SDK v1.2 caused by the new Kconfig flag
  `CONFIG_MEMFAULT_HTTP_USES_MBEDTLS`

### Changes between Memfault SDK 0.30.1 and SDK 0.30.0 - April 6, 2022

#### :chart_with_upwards_trend: Improvements

- Fix stack selection when in ISR context in some Zephyr versions
- Fix a build error when building Zephyr with `CONFIG_NORDIC_SECURITY_BACKEND`
  enabled. New Kconfig flag `CONFIG_MEMFAULT_HTTP_USES_MBEDTLS` can be used to
  manually control this configuration if necessary (default should be set
  correctly in most cases)

#### :house: Internal

- Fix CI unit test build error from older version of gcc

### Changes between Memfault SDK 0.30.0 and SDK 0.29.1 - Mar 31, 2022

#### :rocket: New Features

- Added a Task Watchdog optional module. This can be used to monitor and trigger
  a fault in the case of a task or thread that becomes stuck. See information in
  [components/include/memfault/core/task_watchdog.h](components/include/memfault/core/task_watchdog.h)
  for how to configure and use the module

#### :chart_with_upwards_trend: Improvements

- Fix compilation when building for a Zephyr target that does not have the
  `CONFIG_ARM_MPU` flag enabled
- Fix compilation errors to enable compatibility with Zephyr v3.0.0

### Changes between Memfault SDK 0.29.1 and SDK 0.29.0 - Mar 16, 2022

#### :house: Internal

- Updated Memfault Diagnostic GATT Service (MDS) based on feedback. This service
  can be used to transparently forward data collected by the SDK to a Bluetooth
  Low Energy gateway and proxied to the cloud. See
  [ports/include/memfault/ports/ble/mds.h](ports/include/memfault/ports/ble/mds.h#L1)
- Updated Mbed OS invoke commands to be more resilient against python package
  conflicts

#### :boom: Breaking Changes

- If your project is based on Zephyr < 2.6, you now need to explicitly set
  `CONFIG_OPENOCD_SUPPORT=y` in your `prj.conf`

### Changes between Memfault SDK 0.29.0 and SDK 0.28.2 - Feb 28, 2022

#### :rocket: New Features

- Added a port to Particle's Device OS. More details can be found in
  [`ports/particle/README.md`](ports/particle/README.md).
- Added several more
  [reboot reason options](components/include/memfault/core/reboot_reason_types.h#L16):
  - `kMfltRebootReason_KernelPanic` for explicitly tracking fatal resets from
    within a OS or RTOS
  - `kMfltRebootReason_FirmwareUpdateError` for explicitly tracking resets due
    to firmware update failures or rollbacks

#### :chart_with_upwards_trend: Improvements

- Added a convenience utility function for base64 encoding data in place. See
  [`memfault_base64_encode_inplace`](components/include/memfault/util/base64.h#L35)
  for more details!
- Fixed compiler error in ESP-IDF port when compiling for ESP32-S2 targets

#### :house: Internal

- Added configuration option,
  [`MEMFAULT_COREDUMP_INCLUDE_BUILD_ID`](components/include/memfault/default_config.h#L1),
  which can be used to disable storing the Build Id in a coredump.
- Fixed stale link in Mbed example app [`README`](examples/mbed/README.md).
- Added [utility script](scripts/create_arduino_library.py) that can be used to
  "arduino-ify" the code in this repo.
- Fixed linter errors in python scripts after addition of flake8-bugbear linter
  in CI.
- Fixed compiler error in
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app)
  when compiling with the nRF Connect SDK 1.2 release

### Changes between Memfault SDK 0.28.2 and SDK 0.28.1 - Feb 1, 2022

#### :house: Internal

- Updated
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app) to
  be compatible with nRF Connect SDK 1.9 release
- Updated python scripts to be compatible with new flake8 linter options
- Updated where `lcov` is sourced from when running unit tests in CI

### Changes between Memfault SDK 0.28.1 and SDK 0.28.0 - Jan 20, 2022

#### :chart_with_upwards_trend: Improvements

- Add an optional override flag to control the name used for Zephyr data
  regions- see
  [`ports/zephyr/common/memfault_zephyr_ram_regions.c`](ports/zephyr/common/memfault_zephyr_ram_regions.c).
  Only needed for unusual Zephyr + Memfault configurations prior to Zephyr v2.7
  (for example, nRF Connect SDK v1.7.1 with Memfault SDK v0.27.3+)
- Fix the STM32F7xx reboot reason port to correctly account for the internally
  wired Pin Reset
- Fix a function prototype mismatch in the STM32L4 flash port (thanks to
  @schultetwin for reporting this in #22!)

### Changes between Memfault SDK 0.28.0 and SDK 0.27.3 - Jan 4, 2022

#### :rocket: New Features

- Add support for setting string metrics, see
  `memfault_metrics_heartbeat_set_string()` in
  [`components/include/memfault/metrics/metrics.h`](components/include/memfault/metrics/metrics.h)
  for the new API. See the [Memfault Docs](https://mflt.io/embedded-metrics) for
  general information on using the metrics API.
- Updated `memfault_metrics_heartbeat_debug_print()` to also print current timer
  values, instead of showing `0`. See
  [`components/include/memfault/metrics/metrics.h`](components/include/memfault/metrics/metrics.h)
  for details

#### :chart_with_upwards_trend: Improvements

- Update the STM32 QP/C example ([`examples/qp`](examples/qp)) to compile and
  run correctly now
- Add intructions for exercising Memfault OTA in the ESP32 example, see the
  "Testing OTA" section in
  [`examples/esp32/README.md`](examples/esp32/README.md)
- Update the Memfault HTTP client to URL-encode query params when checking for
  OTA updates (in the case of Device properties containing reserved characters,
  eg `+`). Update the ESP port to check for reserved characters in query params
  and emit an error
- Fix an outdated comment in `cmake/Memfault.cmake`, as reported in
  [issue #21](https://github.com/memfault/memfault-firmware-sdk/issues/21)
  (thank you @C47D !)

#### :house: Internal

- Update Python tests to use a mocked-out gdb instance

### Changes between Memfault SDK 0.27.3 and SDK 0.27.2 - Nov 22, 2021

#### :chart_with_upwards_trend: Improvements

- Fix a build error for the Nordic Connect SDK v1.7.99 development version
- Correct an error in header file include order in the Mynewt port
- Update the esp32 and zephyr examples to use `1.0.0-dev` instead of
  `1.0.0+<6 digits of build id>` for the version specifier. Build id is no
  longer required for symbol file reconciliation and the `+` character is a
  reserved character for URI schemes; this impacted OTA release requests. See
  this document for
  [Memfault's recommended versioning strategy](https://docs.memfault.com/docs/platform/software-version-hardware-version/#software-version)
- Add a reboot reason port for the STM32F7xx family.

#### :house: Internal

- Re-run python `black` and `isort` formatters on python code

### Changes between Memfault SDK 0.27.2 and SDK 0.27.1 - Nov 5, 2021

#### :chart_with_upwards_trend: Improvements

- The Mynewt integration now supports the Memfault demo shell via
  `mflt_shell_init()`, see the [Mynewt port README.md](ports/mynewt/README.md)
  for details. Huge thanks to @t3zeng for providing this implementation!
- Add support for ESP-IDF v4.3.1. This update should fix the bootlooping issue
  seen when using the port with v4.3.1+ of ESP-IDF.
- Add support for `LOG2` deferred mode on zephyr. This should fix bootloops when
  enabling `LOG2`.
- Fix flipped args passed from `MEMFAULT_ASSERT_RECORD()` to
  `MEMFAULT_ASSERT_EXTRA_AND_REASON()` (regression in v0.23.0). This affected
  the `_extra` additional context value passed via this macro.
- Fix a typo in
  [`ports/esp_idf/memfault/common/memfault_platform_http_client.c`](ports/esp_idf/memfault/common/memfault_platform_http_client.c)
  which caused the OTA example to always return "OTA Update Available" when the
  current version is already the latest.

#### :house: Internal

- Updated list of sample apps in (`examples/README.md`)[examples/README.md]

### Changes between Memfault SDK 0.27.1 and SDK 0.27.0 - Oct 11, 2021

#### :chart_with_upwards_trend: Improvements

- Extended mynewt RTOS port to capture coredumps via a
  [`os_coredump_cb`](ports/mynewt/src/memfault_platform_port.c) implementation
  when `MEMFAULT_COREDUMP_CB=1`

#### :house: Internal

- Fixed a few compiler warnings emitted when compiling with clang
- Improved documentation for
  [coredump config with nRF5 SDK](ports/nrf5_sdk/nrf5_coredump_regions.c)

### Changes between Memfault SDK 0.27.0 and SDK 0.26.1 - Oct 5, 2021

#### :chart_with_upwards_trend: Improvements

- Added support for using [compact logs](https://mflt.io/compact-logs) with the
  Memfault [log subsystem](https://mflt.io/logging).
- Added port for mynewt RTOS to Memfault SDK. (Huge thanks to @t3zeng for the
  help here!) See
  [sdk/embedded/ports/mynewt](sdk/embedded/ports/mynewt/README.md) for more
  details.
- Added support for
  [Zephyr 2.7](https://docs.zephyrproject.org/latest/releases/release-notes-2.7.html)
  release.

#### :house: Internal

- Fixed a missing symbol linker error for `memfault_fault_handler` that could
  arise when compiling with `-flto`.
- Fixed a compiler error in `memfault_fault_handling_arm.c` that arose when
  using certain versions of the Clang compiler.
- Cleaned up python scripts after enabling additional PEP8 naming convention
  linters.

### Changes between Memfault SDK 0.26.1 and SDK 0.26.0 - Sept 20, 2021

#### :house: Internal

- Updated
  [`modem_key_mgmt_exists`](ports/zephyr/ncs/src/memfault_nrf91_root_cert_storage.c)
  API usage to be compatible with changes upcomming in nRF Connect SDK 1.8.

### Changes between Memfault SDK 0.26.0 and SDK 0.25.0 - Sept 15, 2021

#### :chart_with_upwards_trend: Improvements

- Added preview of the Memfault Diagnostic GATT Service (MDS). This service can
  be used to transparently forward data collected by the SDK to a Bluetooth Low
  Energy gateway and proxied to the cloud. See
  [ports/include/memfault/ports/ble/mds.h](ports/include/memfault/ports/ble/mds.h#L1)
  for more details!
  - Added reference port of MDS for the DA1469x SDK to
    [ports/dialog/da1469x/memfault_diagnostic_service.c](ports/dialog/da1469x/memfault_diagnostic_service.c#L1)
- Implemented [utility python script](scripts/eclipse_patch.py#L1) which can be
  used for quickly adding a `memfault-firmware-sdk` port to an eclipse project.
  For more details, run

  ```bash
  python scripts/eclipse_patch.py --help
  ```

- Added example project demonstrating integration of Memfault on Cypress'
  [CY8CKIT-064S0S2-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)
  running [Amazon-FreeRTOS](https://github.com/aws/amazon-freertos) publishing
  data using an
  [AWS IoT MQTT broker](https://docs.aws.amazon.com/freertos/latest/userguide/getting_started_cypress_psoc64.html).
  For more details about how to run the Memfault example, see
  [examples/cypress/CY8CKIT-064S0S2-4343W/README.md](examples/cypress/CY8CKIT-064S0S2-4343W/README.md).
- Fixed a compiler warning emitted when using TI's GCC Compiler as reported by
  @albertskog in
  [issue #18](https://github.com/memfault/memfault-firmware-sdk/issues/18)

#### :house: Internal

- Apply some suggestions emitted by `flake8-pytest-style` linter to
  [scripts/tests/test_memfault_gdb.py](scripts/tests/test_memfault_gdb.py).

### Changes between Memfault SDK 0.25.0 and SDK 0.24.2 - August 30, 2021

#### :chart_with_upwards_trend: Improvements

- Added a workaround to
  [`event_storage.h`](components/include/memfault/core/event_storage.h) to
  prevent compilation errors when using
  [the unity test framework](http://www.throwtheswitch.org/unity) to generate
  mocks.
- Updated [`makefiles/MemfaultWorker.mk`](makefiles/MemfaultWorker.mk) to use
  `sort` to guarantee a deterministic file list order irrespestive of
  [make version](https://savannah.gnu.org/bugs/index.php?52076). A consistent
  order is useful for
  [reproducible builds](https://mflt.io/reproducible-builds).
- Make use of `__has_include()` in Zephy port to remove the requirement of
  always needing to create`memfault_platform_config.h`,
  `memfault_metrics_heartbeat_config.def`, &
  `memfault_trace_reason_user_config.def` for a build to compile. To force a
  compile failure instead when any of these files do not exist, a user can set
  [`CONFIG_MEMFAULT_USER_CONFIG_SILENT_FAIL=n`](ports/zephyr/Kconfig)

#### :house: Internal

- The current version of the Memfault Firmware SDK can now be accessed
  programmatically from the
  [`memfault/version.h`](components/include/memfault/version.h).
- Improved HTTP util parser when dealing with malformed status codes
- Updated
  [nRF91 sample test app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app) to
  be compatible with nRF Connect SDK 1.6

### Changes between Memfault SDK 0.24.2 and SDK 0.24.1 - August 17, 2021

#### :chart_with_upwards_trend: Improvements

- Added a new utility API, `memfault_packetizer_set_active_sources`, to the
  [data_packetizer](components/include/memfault/core/data_packetizer.h) module,
  which let's one control the exact data sources that will get packetized. See
  description in the header for more details
- Port Improvements:
  - NXP i.MX RT102x
    - [rich reboot reason info derived from SRC_SRSR register](ports/nxp/rt1021/src_reboot_tracking.c)

### Changes between Memfault SDK 0.24.1 and SDK 0.24.0 - August 9, 2021

#### :chart_with_upwards_trend: Improvements

- Applied suggestions from @elliot-wdtl for the Zephyr ports
  ([#15](https://github.com/memfault/memfault-firmware-sdk/pull/15)):
  - Updated software watchdog port to make use of `MEMFAULT_SOFTWARE_WATCHDOG`
    macro
- Applied suggestions from @ioannisg & @mbolivar-nordic in
  ([#14](https://github.com/memfault/memfault-firmware-sdk/pull/14)) to change
  the KConfig options used to select `CONFIG_MEMFAULT_HTTP_ENABLE` &
  `CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM` &
  `CONFIG_MEMFAULT_NRF_CONNECT_SDK` when using nRF91 targets.

#### :house: Internal

- Added `export` command to the demo cli to better mirror
  [our suggested integration test commands](https://mflt.io/mcu-test-commands)

#### :boom: Breaking Changes

- If you are were using a custom nRF91 based board config (i.e neither
  `BOARD_NRF9160DK_NRF9160NS` or `BOARD_THINGY91_NRF9160NS`), the following
  KConfig options will now be enabled by default. The following can be added to
  your `prj.conf` to restore the original behavior:
  - `CONFIG_MEMFAULT_HTTP_ENABLE=n`
  - `CONFIG_MEMFAULT_NRF_CONNECT_SDK=n`
  - `CONFIG_MEMFAULT_ROOT_CERT_STORAGE_TLS_CREDENTIAL_STORAGE=y`

### Changes between Memfault SDK 0.24.0 and SDK 0.23.0 - July 27, 2021

#### :chart_with_upwards_trend: Improvements

- Added "compact log" support to trace events. When enabled, the format string
  will be removed at compile time from calls to `MEMFAULT_TRACE_EVENT_WITH_LOG`
  and an integer along with arguments will be serialized instead. The actual
  string will recovered and formatted when it arrives in the Memfault cloud.
  This leads to a massive reduction in space & bandwidth needed to send trace
  events. For more details about how to set up,
  [check out this guide!](https://mflt.io/compact-logs)
- Fixed a `-Wshadow` compiler error that would arise in
  [`memfault_coredump_regions_armv7.c`](components/panics/src/memfault_coredump_regions_armv7.c)
  when `MEMFAULT_COLLECT_MPU_STATE` was enabled
- Updated debug print utility in
  [`memfault_coredump_storage_debug.c`](components/panics/src/memfault_coredump_storage_debug.c)
  to guard against potentially printing an uninitialized string.
- Removed unnecessary extra argument from `MEMFAULT_SOFTWARE_WATCHDOG`

#### :boom: Breaking Changes

- If you were already using `MEMFAULT_SOFTWARE_WATCHDOG`, you will need to
  update your call site invocations to remove the argument being passed. i.e

```diff
-      MEMFAULT_SOFTWARE_WATCHDOG(0);
+      MEMFAULT_SOFTWARE_WATCHDOG();
```

### Changes between Memfault SDK 0.23.0 and SDK 0.22.0 - July 8, 2021

#### :chart_with_upwards_trend: Improvements

- Support for Dialog DA1469x chip family (Huge thanks to @iandmorris for the
  help here!)
  - Example eclipse project and more details about how to add the port to any
    DA1469x based project [can be found here](examples/dialog/da1469x).
- Added a simple utility to track heap allocations. This can be used to more
  easily debug what allocations led to out of memory situations with the
  addition of only several hundred bytes to a coredump capture. For more
  details, see
  [`memfault/core/heap_stats.h`](components/include/memfault/core/heap_stats.h)
  - For FreeRTOS users, automatic tracking of heap allocations can be enabled
    with the Memfault port. To enable, see the "Heap Tracking" section in the
    README at [ports/freertos/](ports/freertos).
- Added new convenience utilities for asserting when a watchdog event is
  detected,
  [`MEMFAULT_SOFTWARE_WATCHDOG`](components/include/memfault/panics/assert.h#L65).
  This will result in the issue in the Memfault UI being classified for as a
  "Software Watchdog" instead of an "Assert" for easier classification.
- Fixed a :bug: in
  [Zephyr port](ports/zephyr/common/memfault_platform_metrics.c) where cpu
  runtime metrics were never getting reset after a heartbeat was collected
  leading to always increasing runtime values getting reported.

#### :house: Internal

- Improved support for running [tests](tests/) against different versions of
  clang and gcc and enabled more address sanitizers such as
  [`-fsanitize=undefined`](https://interrupt.memfault.com/blog/ubsan-trap)
- Misc documentation edits

### Changes between Memfault SDK 0.22.0 and SDK 0.21.1 - June 17, 2021

#### :chart_with_upwards_trend: Improvements

- Reduced code space utilized by metric subsystem by transitioning from a string
  representation of metric names to an enum representation.
- Updated [`memfault_gdb.py`](scripts/memfault_gdb.py) helper script to use
  latest Memfault API for uploading symbol files.
- Removed "DST Root CA X3" from the required Memfault
  [root certificate list](components/include/memfault/http/root_certs.h) as
  there is no infrastructure that relies on it anymore.
- Updated PEM representation of all root certificates to include newlines after
  64-character intervals to improve portability with various TLS stacks.

#### :house: Internal

- Updated [`fw_build_id.py`](scripts/fw_build_id.py) script. The same script can
  now also be installed via [`pypi`](https://pypi.org/project/mflt-build-id/):
  `pip install mflt_build_id`
- Various improvements to example app documentation

### Changes between Memfault SDK 0.21.1 and SDK 0.21.0 - June 9, 2021

#### :chart_with_upwards_trend: Improvements

- Zephyr / nRF Connect SDK port:
  - Made periodic upload a named choice,
    `MEMFAULT_HTTP_PERIODIC_UPLOAD_CONTEXT`, so the default can be overriden
    from other Kconfig files.
  - Added prompt text for `MEMFAULT_HTTP_DEDICATED_WORKQUEUE_STACK_SIZE`

### Changes between Memfault SDK 0.21.0 and SDK 0.20.2 - June 8, 2021

#### :chart_with_upwards_trend: Improvements

- Zephyr / nRF Connect SDK port:
  - `CONFIG_MEMFAULT_NRF_CONNECT_SDK` is now only enabled by default when
    `CONFIG_MEMFAULT=y`
  - `CONFIG_DEBUG_THREAD_INFO=y` is now selected by default (in prep for
    deprecation of `CONFIG_OPENOCD_SUPPORT`)
  - Added new option (`CONFIG_MEMFAULT_USER_CONFIG_ENABLE=y`) which can be used
    to remove requirement of providing any user specific configuration.
  - Added two choices for periodic posting of memfault data:
    - `CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_SYSTEM_WORKQUEUE`. This is the
      default and matches previous release behavior of posting data to Memfault
      from the system work queue
    - `CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE` This is a
      new option which can be used to post data from a dedicated worker queue.
      This can be useful if the network stack may block for extended periods of
      time which would stall other system work queue jobs from getting
      processed.

#### :boom: Breaking Changes

- The Zephyr & nRF Connect SDK ports will now only enable the `MEMFAULT_SHELL`
  by default when the Zephyr shell is enabled. `CONFIG_SHELL=y` must now be
  enabled explicitly in your `prj.conf` for the Memfault Shell to be enabled.

### Changes between Memfault SDK 0.20.2 and SDK 0.20.1 - June 4, 2021

#### :house: Internal

- Updated `sMemfaultBuildIdStorage` structure to track the build id length used
  for event serialization and updated [`fw_build_id.py`](scripts/fw_build_id.py)
  script to extract information.

### Changes between Memfault SDK 0.20.1 and SDK 0.20.0 - May 28, 2021

#### :chart_with_upwards_trend: Improvements

- Zephyr / nRF Connect SDK port:
  - Replaced `MEMFAULT_DEFAULT_REBOOT_REASON_IMPL` Kconfig option with
    `MEMFAULT_REBOOT_REASON_GET_CUSTOM` and updated default configuration for
    the nRF Connect SDK. This fixes an issue resulting in the
    [generic memfault_reboot_reason_get](ports/zephyr/common/memfault_platform_core.c#L53)
    getting linked rather than the
    [nRF Connect SDK port](ports/zephyr/ncs/src/nrfx_pmu_reboot_tracking.c#L139).

### Changes between Memfault SDK 0.20.0 and SDK 0.19.0 - May 27, 2021

#### :chart_with_upwards_trend: Improvements

- Updated
  [memfault_fault_handling_arm.c](components/panics/src/memfault_fault_handling_arm.c)
  to work around a compiler bug when using 6.x of the GNU ARM Toolchain
- Port Improvements:
  - SAML10/SAML11
    - [rich reboot reason info derived from RCAUSE register](ports/atmel/saml1x/rcause_reboot_tracking.c#L1)
  - Updated [esp-idf port](ports/esp_idf) to streamline integrations making use
    of the [amazon-freertos](https://github.com/aws/amazon-freertos)
  - Zephyr
    - Added several Kconfig options for better control over information
      collected in a coredump:
      - `CONFIG_MEMFAULT_COREDUMP_COLLECT_DATA_REGIONS` to enable/disable
        collection of `.data` region
      - `CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS` to enable/disable
        collection of `.bss` region
      - `CONFIG_MEMFAULT_COREDUMP_STORAGE_CUSTOM=y` can be used to opt out of
        the default RAM backed coredump implementation and provide a custom one
        in the port.
- Reduced instruction cycles required to update a
  [heartbeat metric](metrics/src/memfault_metrics.c)
- Events will now store an abbreviated build id when serialized

### Changes between Memfault SDK 0.19.0 and SDK 0.18.0 - May 19, 2021

#### :chart_with_upwards_trend: Improvements

- Added support for collecting additional register information when a Hardfault
  takes place when using the Zephyr port. This information will be decoded and
  displayed in the Memfault UI in the "Exceptions" tab.

- Updated
  [`buffered_coredump_storage.h` ](ports/include/memfault/ports/buffered_coredump_storage.h)
  to use `memmov` instead of `memcpy` since `dst` and `src` buffers may overlap
  when all of `.bss` is saved in a coredump capture.
- Added a new Kconfig option to the Zephyr port,
  `CONFIG_MEMFAULT_METRICS_EXTRA_DEFS_FILE=y`, which causes
  `memfault_metrics_heartbeat_extra.def` to be included in the metric
  definitions. This can be utilized by a third party consumer of Zephyr to more
  easily extend the default heartbeat metrics collected when using memfault.

#### :house: Internal

- Updated [`memfault_gdb.py`](scripts/memfault_gdb.py) helper script to use
  latest Memfault API for uploading symbol files.

#### :boom: Breaking Changes

- If you are using [nRF Connect SDK / Zephyr port](ports/zephyr/ncs/), the SDK
  will now automatically be picked up as a Zephyr Module! You will need to make
  two changes:
  1. Remove the `ZEPHYR_EXTRA_MODULES` addition from your projects
     CMakeLists.txt, i.e
  ```diff
  --- a/your_application/CMakeLists.txt
  +++ b/your_application/CMakeLists.txt
  @@ -3,7 +3,6 @@
  - list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../modules/memfault-firmware-sdk/ports/nrf-connect-sdk)
  ```
  2. Add `CONFIG_MEMFAULT_NRF_CONNECT_SDK=y` to your projects `prj.conf`

### Changes between Memfault SDK 0.18.0 and SDK 0.17.1 - May 14, 2021

#### :chart_with_upwards_trend: Improvements

- Support for Dialog DA145xx chip family (Huge thanks to @iandmorris for the
  help here!)
  - GCC & Keil based demo application for the DA14531 & DA14585/DA14586
    [can be found here](examples/dialog/da145xx).
  - Ports for applications using the DA145xx SDK
    [can be found here](ports/dialog/da145xx).
- ESP32 port improvements
  - Added example of periodically posting data to memfault via a background
    task.
  - Added a new Kconfig option, `MEMFAULT_COREDUMP_USE_OTA_SLOT=y` which can be
    used to save a coredump in an unused OTA slot rather than the default
    coredump partition. This can be useful in situations where Memfault is being
    integrated after a product has shipped and updating the partition table is
    no longer possible.
- Added `MEMFAULT_EVENT_STORAGE_NV_SUPPORT_ENABLED=0` which can be used to
  disable dynamic configuration of non-volatile storage. Setting this flag when
  the non-volatile event storage API is not in use will save several hundred
  bytes of codespace.
- Hardened
  [memfault_http_parse_response()](components/http/src/memfault_http_utils.c)
  utility to parse HTTP responses with headers that exceed a length of 128 bytes
- Fixed a :bug: in`memfault_log_save_preformatted()` leading to invalid logs
  being reported when attempting to save log lines > 128 bytes. (thanks @alvarop
  for the report!)
- Added a convenience API,
  [`memfault_create_unique_version_string()`](components/include/memfault/core/platform/device_info.h),
  which can be used for easily appending a build id on the software version
  reported.

#### :house: Internal

- Updates to demo cli:
  - `MEMFAULT_DEMO_SHELL_RX_BUFFER_SIZE` can be used to shrink the maximum
    amount of bytes that can be buffered on a single line.
  - Made `memfault_demo_shell_commands.h` public and moved it to
    [`memfault/demo/shell_commands.h`](components/include/memfault/demo/shell_commands.h)
    to facilitate easier overriding of the default set of commands used in a
    build.

### Changes between Memfault SDK 0.17.1 and SDK 0.17.0 - April 30, 2021

#### :chart_with_upwards_trend: Improvements

- ESP32 Example App Updates:
  - Added `export` command to demonstrate how data can be dumped via the console
  - Added
    [Memfault Build ID](examples/esp32/apps/memfault_demo_app/CMakeLists.txt) to
    example app as a reference
- Fixed a :bug: in
  [`memfault_platform_sanitize_address_range()`](ports/templates/memfault_platform_port.c)
  template example.
- Refactored nRF5 example app to mirror integration steps listed in the
  [latest integration guide](https://mflt.io/cortex-m-getting-started)
- Added a new configuration option, `MEMFAULT_PLATFORM_FAULT_HANDLER_CUSTOM`,
  which can be used to explicitly disable the stub
  [`memfault_platform_fault_handler()` implementation](components/panics/src/memfault_fault_handling_arm.c)
- Improve the quality of backtrace recovery for asserts when using Arm Compiler
  5 by removing use of noreturn function attribute for
  `memfault_fault_handling_assert()` declaration.

### Changes between Memfault SDK 0.17.0 and SDK 0.16.1 - April 26, 2021

#### :rocket: New Features

- Added support for collecting ARMv7-M MPU regions as part of coredump
  collection. To enable, set
  [`MEMFAULT_COLLECT_MPU_STATE=1`](components/include/memfault/default_config.h#L1)
  in your `memfault_platform_config.h`. Once enabled, the MPU will be
  automatically analyzed for configuration errors and results will be presented
  in the "MPU" tab in the Memfault UI for a coredump.
- Added a new API,
  [`memfault_log_trigger_collection`](components/include/memfault/core/log.h#L143),
  which can be used to "freeze" the current contents of the log buffer when
  unexpected behavior takes place on the device for upload to Memfault. The logs
  can then be [uploaded to Memfault](https://mflt.io/data-to-cloud) just like
  any other data and appear in the UI for a device. For more details about the
  Memfault log subsystem see https://mflt.io/logging

#### :chart_with_upwards_trend: Improvements

- Fixed compilation error when compiling the panics component against Cortex-M0+
  with ARM Compiler 5
- Added several
  [default heartbeat metrics to the Zephyr port](ports/zephyr/config/memfault_metrics_heartbeat_zephyr_port_config.def)
  around timer task stack usage and execution time.

#### :house: Internal

- Refreshed esp32 example app README and updated instructions for the v3.3.5
  esp-idf
- Added `test_log` & `trigger_logs` CLI commands to nRF5 & Zephyr example
  applications to exercise new log collection functionality.

### Changes between Memfault SDK 0.16.1 and SDK 0.16.0 - April 12, 2021

#### :chart_with_upwards_trend: Improvements

- Fixed a :bug: in Zephyr port leading to a compilation error with nRF Connect
  SDK when `CONFIG_DOWNLOAD_CLIENT=y` & `CONFIG_MEMFAULT_NRF_SHELL=y`
- Dialog DA1468x
  [QSPI coredump storage port](ports/dialog/da1468x/qspi_coredump_storage.c#L1)
  updates:
  - default storage parition (`NVMS_LOG_PART`) can be overridden
    `MEMFAULT_PLATFORM_COREDUMP_STORAGE_PARTITION`
  - Max space used within partition can be limited using
    `MEMFAULT_PLATFORM_COREDUMP_STORAGE_MAX_SIZE_BYTES`
- Updated [zephyr example application](examples/zephyr/README.md) and docs to be
  compatible with v2.5 release.

#### :house: Internal

- Added `memfault_log_save` stub to unit tests to facilitate easier testing of
  logging dependencies
- Improved correctness of strategy used to capture `msp` & `psp` register in
  Cortex-M fault handler.
- Added new convenience utility,
  `memfault_circular_buffer_read_with_callback()`, to
  [circular buffer api](components/include/memfault/util/circular_buffer.h).

### Changes between Memfault SDK 0.16.0 and SDK 0.15.0 - April 8, 2021

#### :chart_with_upwards_trend: Improvements

- Added new convenience APIs, `memfault_event_storage_bytes_used()` &
  `memfault_event_storage_bytes_free()`, to
  [event storage module](components/include/memfault/core/event_storage.h#L1).
- Added a new configuration option,
  [`MEMFAULT_ASSERT_HALT_IF_DEBUGGING_ENABLED`](components/include/memfault/default_config.h#L1).
  By default, it is off, but when enabled will cause
  `memfault_platform_halt_if_debugging()` to be called prior to triggering the
  full coredump capture.
- Fixed a type conversion compiler warning emitted by IAR and ARMCC in
  [`resetreas_reboot_tracking.c`](ports/nrf5_sdk/resetreas_reboot_tracking.c#L1).
- Port Updates:
  - Dialog DA1468x
    - [QSPI coredump storage port](ports/dialog/da1468x/qspi_coredump_storage.c#L1)
    - [Added `memfault_platform_reboot_tracking_boot()` implementation](ports/dialog/da1468x/reset_stat_reboot_tracking.c)

#### :house: Internal

- Removed "Heartbeat triggered!" print when
  `memfault_metrics_heartbeat_debug_trigger()` is called

#### :boom: Breaking Changes

- If you were using
  [`ports/dialog/da1468x/reset_stat_reboot_tracking.c`](ports/dialog/da1468x/reset_stat_reboot_tracking.c),
  the `memfault_platform_reboot_tracking_boot()` implementation from your
  `memfault_platform_port.c` file can be removed and the one in the port can be
  picked up.

### Changes between Memfault SDK 0.15.0 and SDK 0.14.0 - March 24, 2021

#### :chart_with_upwards_trend: Improvements

- Added a new convenience API,
  [`memfault_device_info_dump()`](components/include/memfault/core/device_info.h#L1)
  which can be used to pretty print the device information populated in the
  `memfault_platform_get_device_info()` dependency function.
- Added
  [`memfault_platform_sanitize_address_range()`](components/include/memfault/panics/platform/coredump.h#L95).
  This functrions is intended for use in your
  `memfault_platform_coredump_get_regions()` implementation when capturing
  regions that are not defined at compile time.
- Fixed a :bug: with `memfault_coredump_storage_debug_test_*` which would
  generate a false positive test failure when the coredump storage area was not
  divisible by 16.
- C++ header guards are now included in all headers in the ports/ directory for
  easier integration in mixed C/C++ environments
- Port Updates:
  - Dialog DA1468x: Added patch which can be used to add
    [GNU Build ID](ports/dialog/da1468x/gnu-build-id.patch#L1)
  - Added support for FreeRTOS 8 to the [FreeRTOS port](ports/freertos/).
  - STM32H7 family / STM32CubeH7:
    - [rich reboot reason info derived from RCC RSR register](ports/stm32cube/h7/rcc_reboot_tracking.c#L1)
  - STM32WBxx family / STM32CubeWB:
    - [internal flash coredump storage](ports/stm32cube/wb/flash_coredump_storage.c#L1)
  - Improved configurability of
    [RAM backed coredump storage port](ports/panics/src/memfault_platform_ram_backed_coredump.c#L1)
    with new configuration options for to control where RAM is allocated, what
    memory regions are collected, and stack size to collect.
- Added additional comments to [`ports/templates`](ports/templates) directory to
  facilitate porting.

#### :house: Internal

- [Demo CLI Shell commands](components/demo/src/memfault_demo_shell_commands.c#L50)
  are now defined as weak symbols so they can be overriden with a custom set.

#### :boom: Breaking Changes

- If you were using the
  [`ports/emlib/msc_coredump_storage.c`](ports/emlib/msc_coredump_storage.c)
  port in your system, you must add
  `MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_FLASH=1` to your
  `memfault_platform_config.h`
- If you were using `ports/panics/src/memfault_platform_ram_backed_coredump.c`:
  - [`memfault_platform_sanitize_address_range()`](components/include/memfault/panics/platform/coredump.h)
    must be implemented.
  - The default coredump storage RAM size was changed from 700 to 1024 bytes so
    more information can be captured. The original size can be restored by
    setting `MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 700` in your
    `memfault_platform_config.h`

### Changes between Memfault SDK 0.14.0 and SDK 0.13.1 - March 18, 2021

#### :chart_with_upwards_trend: Improvements

- Renamed `platforms` folder to [`examples`](examples/) to better capture that
  the folder contains "example" integrations of the Memfault SDK into hello
  world style apps from various SDKs
- Port Updates:
  - Zephyr: added a
    [software watchdog port](ports/zephyr/common/memfault_software_watchdog.c#L1)
    which can be used for capturing coredumps ahead of a hardware watchdog
    reset.
  - EFR32:
    [rich reboot reason info derived from EMU_RSTCAUSE register](ports/emlib/rmu_reboot_tracking.c#L1)
- Updated [nRF9160 Demo Application](examples/nrf-connect-sdk/nrf9160) to be
  compatible with
  [nRF Connect SDK 1.5](https://github.com/nrfconnect/sdk-nrf/tree/v1.5-branch)
- Added support for capturing coredumps with GDB from GNU GCC 4.9 to
  [`memfault_gdb.py`](scripts/memfault_gdb.py)

#### :house: Internal

- Added support for automatically capturing logs with Zephyr when logging in
  synchronous mode `CONFIG_LOG_IMMEDIATE=y`
- Unified nomenclature for references to the "Project Key" used to push data to
  Memfault

#### :boom: Breaking Changes

If you were linking any files from the `platforms` folder into your project, the
path needs to be updated to `examples`:

```diff
- ${MEMFAULT_FIRMWARE_SDK}/platforms/
+ ${MEMFAULT_FIRMWARE_SDK}/examples/
```

### Changes between Memfault SDK 0.13.1 and SDK 0.13.0 - March 10, 2021

#### :chart_with_upwards_trend: Improvements

- Reference platform API implementations for DA1468x:
  - [rich reboot reason info derived from RESET_STAT_REG register](ports/dialog/da1468x/reset_stat_reboot_tracking.c#L1)
- Fixed a :bug: that led to a unit test failure in `test_coredump_storage_debug`
  in some environments where `const` arrays were getting dynamically built on
  the stack at runtime.
- Worked around a limitation in GNU GCC 4.9's extended ASM to fix a compiler bug
  that would arise when compiling `memfault_fault_handling_arm.c` for Cortex-M0
  targets.
- Added a new [`ports/templates`](ports/templates) folder that can be
  copy/pasted into a project and used as a starting point for a Memfault port!

### Changes between Memfault SDK 0.13.0 and SDK 0.12.0 - March 4, 2021

#### :chart_with_upwards_trend: Improvements

- Improved documentation in [README](README.md) and [components](components/)
  directories.
- Added coredump capture support to panics component for the AARCH64
  architecture.
- Reference platform API implementations for the following MCUs/SDKs:
  - STM32WBxx family / STM32CubeWB:
    - [rich reboot reason info derived from RCC CSR register](ports/stm32cube/wb/rcc_reboot_tracking.c#L1)
  - nRF5 SDK
    - [app_timer based port for metric dependencies](ports/nrf5_sdk/memfault_platform_metrics.c#L1)
- [`fw_build_id.py`](scripts/fw_build_id.py) script improvements
  - script is now compatible with Python2 environments.
  - Added new `--dump <chars>` option to simplify extraction of build id in
    automation, i.e:
    ```
     python scripts/fw_build_id.py <ELF> --dump 7
     3a3e81f
    ```

#### :house: Internal

- Started work to enable automatic capture of logs in a coredump for the Zephyr
  & nRF Connect SDK.

#### :boom: Breaking Changes

- If you were linking `memfault_nrf5_coredump.c` in your project:
  - the file has been split into `nrf5_coredump_regions.c` (which defines the
    regions to collect in a coredump) & `nrf5_coredump_storage.c` (which
    implements the port for saving coredumps to internal flash). Both of these
    files must be added to your build system.
  - Linker script names for region to save coredump to and regions to collect
    need to be updated:
    - Update`__CoreStart` & `__MemfaultCoreStorageEnd` to
      `__MemfaultCoreStorageStart` & `__MemfaultCoreStorageEnd` in linker
      script.
    - Update `__MfltCoredumpRamStart` & `__MfltCoredumpRamEnd` to
      `__MemfaultCoredumpRamStart` & `__MemfaultCoredumpRamEnd`

### Changes between Memfault SDK 0.12.0 and SDK 0.11.4 - Feb 14, 2021

#### :chart_with_upwards_trend: Improvements

- The SDK now includes a central configuration header at
  [`components/include/memfault/config.h`](components/include/memfault/config.h#L1).
  Platform overrides can be defined in `memfault_platform_config.h`
- Reference platform API implementations for the
  [nRF5 SDK](https://infocenter.nordicsemi.com/topic/struct_sdk/struct/sdk_nrf5_latest.html)
  - [rich reboot reason info derived from RESETREAS register](ports/nrf5_sdk/resetreas_reboot_tracking.c#L1)
  - [internal flash coredump storage](ports/nrf5_sdk/memfault_nrf5_coredump.c#L1)
- All headers for Memfault "components" can now be found under
  [`components/include/memfault/`](components/include/memfault/). This
  simplifies adding components to build systems as only a single path is now
  needed!

#### :boom: Breaking Changes

- You must create the file `memfault_platform_config.h` and add it to your
  include path. This file can be used in place of compiler defines to tune the
  SDK configurations settings.
- If you are not using a Memfault
  [build system helper](README.md#add-sources-to-build-system)
  1. Remove all include paths at
     `${MEMFAULT_FIRMWARE_SDK}/components/${COMPONENT}/include`
  2. Add include path `${MEMFAULT_FIRMWARE_SDK}/components/include` to your
     build system
- If you were linking any of the nRF5 example app files in your project, the
  directory has changed:

```diff
- ${MEMFAULT_FIRMWARE_SDK}/nrf5/libraries/memfault/platform_reference_impl/memfault_platform_reboot_tracking.c
+ ${MEMFAULT_FIRMWARE_SDK}/ports/nrf5_sdk/resetreas_reboot_tracking.c
- ${MEMFAULT_FIRMWARE_SDK}/nrf5/libraries/memfault/platform_reference_impl/memfault_platform_coredump.c
+ ${MEMFAULT_FIRMWARE_SDK}/ports/nrf5_sdk/memfault_nrf5_coredump.c
```

### Changes between Memfault SDK 0.11.4 and SDK 0.11.3 - Feb 4, 2021

- ESP8266 Updates
  - Added new Kconfig option which can be set via `make menuconfig` and be used
    to disable the Memfault integration, `CONFIG_MEMFAULT=n`.
  - Fixed a :bug: leading to a compilation error when
    both`CONFIG_USING_ESP_CONSOLE=n` and `CONFIG_MEMFAULT_CLI_ENABLED=n`
- Added default implementations for `MEMFAULT_GET_LR()`, `MEMFAULT_GET_PC()`,
  and `MEMFAULT_BREAKPOINT()` to
  [`compiler_gcc.h`](components/include/memfault/core/compiler_gcc.h) to
  facilitate compilations of the SDK against other architectures.

### Changes between Memfault SDK 0.11.3 and SDK 0.11.2 - Jan 31, 2021

#### :chart_with_upwards_trend: Improvements

- Reference platform API implementations for the following MCUs/SDKs:
  - nRF Connect SDK
    - Added support for nRF Connect SDK v1.3.x
    - Added support for FOTA with Memfault. See
      [`memfault_fota_start()`](ports/nrf-connect-sdk/zephyr/include/memfault/nrfconnect_port/fota.h)
      for more details
  - Zephyr
    - Added implementations for `memfault_platform_metrics_timer_boot()` &
      `memfault_platform_get_time_since_boot_ms()` memfault dependencies to
      Zephyr port. A custom implementation can still be provided by
      setting`MEMFAULT_METRICS_TIMER_CUSTOM=y`
    - Metrics support is now enabled by default when `CONFIG_MEMFAULT=y` and can
      be disabled by setting `CONFIG_MEMFAULT_METRICS=n`
    - Added support for periodically uploading Memfault data in the background.
      This is off by default and can be enabled with the
      `CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD=y` option
- Fixed a :bug: that could lead to an invalid coredump being sent to Memfault
  when `memfault_packetizer_abort()` was called after a coredump was partially
  sent.

### Changes between Memfault SDK 0.11.2 and SDK 0.11.1 - Jan 21, 2021

#### :chart_with_upwards_trend: Improvements

- Added support for nRF Connect SDK v1.2.x and updated the
  [integration guide](https://mflt.io/nrf-connect-sdk-integration-guide)
  accordingly.

### Changes between Memfault SDK 0.11.0 and SDK 0.10.1 - Jan 19, 2021

#### :rocket: New Features

- Added full support for the ESP8266 (Tensilica Xtensa LX106) MCU architecture!
  A step-by-step integration guide can be found
  [here](https://mflt.io/esp8266-tutorial) and the port to the ESP8266 RTOS SDK
  can be found [here](ports/esp8266_sdk/).

#### :chart_with_upwards_trend: Improvements

- Added a convenience header for picking up includes for all Memfault
  components. If you are using the
  [build system helpers](README.md#add-sources-to-build-system), this path will
  be picked up automatically.

```c
#include "memfault/components.h"

// call to any Memfault API in the components folder
```

- Fixed a :bug: leading to Root CAs not get loaded correctly when using the nRF
  Connect SDK port and the `MEMFAULT_ROOT_CERT_STORAGE_TLS_CREDENTIAL_STORAGE=y`
  Kconfig option.
- Applied suggestions from @rerickson1 for the Zephyr and nRF Connect SDK ports:
  - [`CONFIG_MEMFAULT_METRICS=y`](https://github.com/memfault/memfault-firmware-sdk/pull/8)
    can now be used to compile the metrics component into the Zephyr and nRF
    Connect SDK ports.
  - [`CONFIG_MEMFAULT=y`](https://github.com/memfault/memfault-firmware-sdk/pull/7)
    must now be specified to enable the Memfault integration.

#### :house: Internal

- Refactored the [nRF Connect SDK port](ports/nrf-connect-sdk) to build directly
  on top of the [zephyr port](ports/zephyr) reducing code duplication and
  facilitate the rollout of additional features to both SDKs at the same time
  moving forward.

#### :boom: Breaking Changes

- If you are using a Memfault
  [build system helper](README.md#add-sources-to-build-system) _and_ not using
  the trace or metrics functionality, you will now need to create a
  `memfault_trace_reason_user_config.def` or
  `memfault_metrics_heartbeat_config.def` file, respectively, and add it to your
  include path.
- When using the Zephyr port, the memfault integration must be enabled
  explicitly by adding `CONFIG_MEMFAULT=y` to your `prj.conf`

### Changes between Memfault SDK 0.10.1 and SDK 0.10.0 - Jan 10, 2021

#### :chart_with_upwards_trend: Improvements

- Reference platform API implementations for the following MCUs/SDKs:
  - STM32F4 family / STM32CubeF4:
    - [rich reboot reason info derived from RCC CSR register](ports/stm32cube/f4/rcc_reboot_tracking.c)
  - STM32L4 family / STM32CubeL4
    - [rich reboot reason info derived from RCM SRS register](ports/stm32cube/l4/rcc_reboot_tracking.c)
  - nRF Connect SDK
    - [rich reboot reason info derived from PMU RESETREAS register](ports/nrf-connect-sdk/nrfx/pmu_reboot_tracking.c)
    - refactored HTTP port to support multiple backing storage strategies for
      root certificates. See
      [`MEMFAULT_ROOT_CERT_STORAGE_*`](ports/nrf-connect-sdk/zephyr/Kconfig)
      Kconfig options for more details
    - Added support for Memfault OTA downloads. See
      [memfault_nrfconnect_port_ota_update()](ports/nrf-connect-sdk/zephyr/include/memfault/nrfconnect_port/http.h)
      for more details

### Changes between Memfault SDK 0.10.0 and SDK 0.9.2 - Jan 5, 2021

#### :chart_with_upwards_trend: Improvements

- Updated
  [`memfault_freertos_ram_regions.c`](ports/freertos/src/memfault_freertos_ram_regions.c)
  port to collect all TCBs and then stacks. This way, the state of all tasks can
  be recovered even if the coredump storage regin is filled while writing all
  the task stacks.
- Reference platform API implementations for the following MCUs/SDKs:
  - STM32F4 family / STM32CubeF4
    - [internal flash coredump storage](ports/stm32cube/f4/flash_coredump_storage.c)
  - STM32L4 family / STM32CubeL4
    - [internal flash coredump storage](ports/stm32cube/l4/flash_coredump_storage.c)
  - NXP's S32K1xx family / S32K1 SDL
    - [internal flash coredump storage using FTFC peripheral](ports/s32sdk/ftfc_flash_coredump_storage.c)
    - [software watchdog implementation using LPIT peripheral](ports/s32sdk/lpit_software_watchdog.c)
    - [rich reboot reason info derived from RCM SRS register](ports/s32sdk/rcm_reboot_tracking.c)
  - Silicon Lab's EFM/EFR family / v3.0 Gecko SDK
    - [internal flash coredump storage using MSC peripheral h](ports/emlib/msc_coredump_storage.c)
    - [software watchdog implementation using warning interrupt in WDOG peripheral](ports/emlib/wdog_software_watchdog.c)
    - [reboot reason info derived from RMU RSTCAUSE register](ports/emlib/rmu_reboot_tracking.c)

#### :rocket: New Features

- Added several more
  [reboot reason options](components/include/memfault/core/reboot_reason_types.h#L16):
  - `kMfltRebootReason_PinReset` for explicitly tracking external pin resets.
  - `kMfltRebootReason_SoftwareWatchdog` & `kMfltRebootReason_HardwareWatchdog`
    for easier disambiguation between watchdog resets where a coredump was
    captured versus ones where no software handler ran and hardware reset the
    device.
  - `kMfltRebootReason_ClockFailure` for explicit tracking of resets due to loss
    of a clock signal or PLL lock.
  - `kMfltRebootReason_Lockup` for explicit tracking of faults from within the
    Hardfault or NMI exceptions on ARM Cortex-M MCUs.
- Added a utility which can be used to verify a platform coredump storage
  implementation is working as corrected. For more details about how to use, see
  [memfault_coredump_storage_debug.c](components/panics/src/memfault_coredump_storage_debug.c#L1).

#### :house: Internal

- Added infrastructure to coredump collection in `panics` component to support
  ESP8266 (Tensilica Xtensa LX106) MCU architecture.

### Changes between Memfault SDK 0.9.3 and SDK 0.9.2 - Dec 14, 2020

#### :chart_with_upwards_trend: Improvements

- nRF Connect Updates:

  - Updated port to support
    [nRF Connect SDK v1.4.0](https://github.com/nrfconnect/sdk-nrf/tree/v1.4-branch)
  - Added port of HTTP client to post data to Memfault
  - Added support for capturing state of all Zephyr tasks during a crash. This
    way the state of all threads can be seen in the Memfault UI when a crash is
    uploaded.
  - Updated
    [memfault_demo_app](examples/nrf-connect-sdk/nrf9160/memfault_demo_app) to
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
  [`MEMFAULT_TRACE_EVENT_WITH_LOG(reason, ...)`](components/include/memfault/core/trace_event.h#L77).
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
  [`components/include/memfault/core/debug_log.h`](components/include/memfault/core/debug_log.h)
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
  [`memfault_platform_coredump_get_regions()`](components/include/memfault/panics/platform/coredump.h)
  so it is recommended to order this list from the most to least important
  regions to capture.
- Updated FreeRTOS port to use static allocation APIs by default when the
  `configSUPPORT_STATIC_ALLOCATION=1` configuration is used.

### Changes between Memfault SDK 0.8.1 and SDK 0.8.0 - Nov 3, 2020

#### :chart_with_upwards_trend: Improvements

- Added several more
  [reboot reason options](components/include/memfault/core/reboot_reason_types.h#L16):
  `kMfltRebootReason_SoftwareReset` & `kMfltRebootReason_DeepSleep`.
- Extended [ESP32 port](https://mflt.io/esp-tutorial) to include integrations
  for [reboot reason tracking](https://mflt.io/reboot-reasons) and
  [log collection](https://mflt.io/logging).
- Apply missing check to unit test
  [reported on Github](https://github.com/memfault/memfault-firmware-sdk/pull/6)

### Changes between Memfault SDK 0.8.0 and SDK 0.7.2 - Oct 26, 2020

#### :chart_with_upwards_trend: Improvements

- Added a new convenience API,
  [`memfault_coredump_storage_check_size()`](components/include/memfault/panics/coredump.h),
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
  [Memfault Build ID](components/include/memfault/core/build_info.h#L8-L42) as a
  string.
- Added default implementations of several Memfault SDK dependency functions
  when using FreeRTOS to [ports/freertos](ports/freertos).

### Changes between Memfault SDK 0.7.2 and SDK 0.7.1 - Sept 1, 2020

#### :chart_with_upwards_trend: Improvements

- A status or error code (i.e bluetooth disconnect reason, errno value, etc) can
  now be logged alongside a trace event by using a new API:
  [`MEMFAULT_TRACE_EVENT_WITH_STATUS(reason, status_code)`](components/include/memfault/core/trace_event.h#L55).

### Changes between Memfault SDK 0.7.1 and SDK 0.7.0 - Sept 1, 2020

#### :chart_with_upwards_trend: Improvements

- Added support for TI's ARM-CGT Compiler
- Removed dependency on NMI Exception Handler for `MEMFAULT_ASSERT`s. Instead of
  pending an NMI exception, the assert path will now "trap" into the fault
  handler by executing a `udf` instruction. This unifies the fault handling
  paths within the SDK and leaves the NMI Handler free for other uses within the
  user's environment.
- Added several more
  [reboot reason options](components/include/memfault/core/reboot_reason_types.h#L16):
  `kMfltRebootReason_PowerOnReset`, `kMfltRebootReason_BrownOutReset`, &
  `kMfltRebootReason_Nmi`.

### Changes between Memfault SDK 0.7.0 and SDK 0.6.1 - Aug 6, 2020

#### :chart_with_upwards_trend: Improvements

- Added utility to facilitate collection of the memory regions used by the
  [logging module](components/include/memfault/core/log.h) as part of a
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
  [memfault/core/data_export.h](components/include/memfault/core/data_export.h#L5)
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
  [memfault/core/build_info.h](components/include/memfault/core/build_info.h#L8-L42).
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
  [unique build id](components/include/memfault/core/build_info.h#L8-L42) to
  your build (several line code change).

### Changes between Memfault SDK 0.4.0 and SDK 0.3.4 - May 6, 2020

#### :rocket: New Features

- Added support for (optionally) storing events collected by the SDK to
  non-volatile storage mediums. This can be useful for devices which experience
  prolonged periods of no connectivity. To leverage the feature, an end user
  must implement the
  [nonvolatile_event_storage platform API](components/include/memfault/core/platform/nonvolatile_event_storage.h#L7).

#### :chart_with_upwards_trend: Improvements

- Added an assert used internally by the SDK which makes it easier to debug API
  misuse during bringup. The assert is enabled by default but can easily be
  disabled or overriden. For more details see
  [`memfault/core/sdk_assert.h`](components/include/memfault/core/sdk_assert.h#L6).
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
  [`memfault_platform_time_get_current()`](components/include/memfault/core/platform/system_time.h#L33).
  If the API is not implemented, the `captured_date` will continue to be set
  based on the time the event was received by the memfault cloud.

#### :chart_with_upwards_trend: Improvements

- Added a reference implementation of
  [reboot reason tracking](https://mflt.io/2QlOlgH) to the
  [NRF52 demo app](examples/nrf5/libraries/memfault/platform_reference_impl/memfault_platform_reboot_tracking.c#L1)
  and a new `reboot` CLI command to easily exercise it.
- A `reset_reason` can now optionally be provided as part of
  [`sResetBootupInfo`](components/include/memfault/core/reboot_tracking.h#L41).
  This can be useful for scenarios where the reboot reason is known on bootup
  but could not be set prior to the device crashing.
- A reboot reason event will now _always_ be generated when
  `memfault_reboot_tracking_boot()` is called even if no information about the
  reboot has been provided. In this scenario, the reset reason will be
  [`kMfltRebootReason_Unknown`](components/include/memfault/core/reboot_reason_types.h#L16)

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

- Added [`memfault_log_read()`](components/include/memfault/core/log.h#L95-L121)
  API to make it possible to use the module to cache logs in RAM and then flush
  them out to slower mediums, such as a UART console or Flash, from a background
  task.

#### :chart_with_upwards_trend: Improvements

- A pointer to the stack frame upon exception entry is now included in
  `sCoredumpCrashInfo` when
  [memfault_platform_coredump_get_regions](components/include/memfault/panics/platform/coredump.h#L56)
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
  [log.h](components/include/memfault/core/log.h).
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
  [metrics.h](components/include/memfault/metrics/metrics.h) for more details.
- If you are _not_ using our CMake or Make
  [build system helpers](README.md#add-sources-to-build-system), you will need
  to add `$(MEMFAULT_SDK_ROOT)/components/core/src/memfault_log.c` to your
  project. (Note that until
  [`memfault_log_boot()`](https://github.com/memfault/memfault-firmware-sdk/blob/master/components/include/memfault/core/log.h#L33-L38)
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
    [here](examples/esp32/README.md).

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
  [memfault/core/data_source_rle.h](components/include/memfault/core/data_source_rle.h).
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
  [data_packetizer](components/include/memfault/core/data_packetizer.h) module.
- Add a new eMfltCoredumpRegionType, `MemoryWordAccessOnly` which can be used to
  force the region to be read 32 bits at a time. This can be useful for
  accessing certain peripheral register ranges which are not byte addressable.
- Automatically collect Exception / ISR state for Cortex-M based targets. NOTE
  that the default config requires an additional ~150 bytes of coredump storage.
  The feature can be disabled completely by adding
  `-DMEMFAULT_COLLECT_INTERRUPT_STATE=0` to your compiler flags. More
  configuration options can be found in
  [memfault_coredump_regions_armv7.c](components/panics/src/memfault_coredump_regions_armv7.c).
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
  See [examples/qp/README.md](examples/qp/README.md) for more details.
- Added demo application and port for Arm Mbed OS 5 running on the
  [STM32F429I-DISC1 evaluation board](https://www.st.com/en/evaluation-tools/32f429idiscovery.html).
  See [examples/mbed/README.md](examples/mbed/README.md) for more details.
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
  [zephyr demo app directory](examples/zephyr/README.md) for more details

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
  running ChibiOS. See `examples/stm32/stm32h743i/README.md` for more details.
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
  (PCA10056) development kit. See `examples/nrf5/README.md` for more details.
- Made SDK headers suitable for includion in C++ files

First Public Release of Memfault Firmware SDK 0.0.2 - June 26, 2019

- published initial Memfault SDK, see `README.md` in root for summary
- published **panics** API which is a C SDK that can be integrated into any
  Cortex-M device to save a "core" (crash state) on faults and system asserts.
  See`components/panics/README.md` for more details
- Added example port and demo of **panics** SDK for the BCM943364WCD1 evaluation
  board running the WICED Wifi stack. More details in `examples/wiced/README.md`
- add python invoke based CLI wrapper for demo ports
