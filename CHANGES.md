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
  [`sResetBootupInfo`](components/panics/include/memfault/panics/reboot_tracking.h#L41).
  This can be useful for scenarios where the reboot reason is known on bootup
  but could not be set prior to the device crashing.
- A reboot reason event will now _always_ be generated when
  `memfault_reboot_tracking_boot()` is called even if no information about the
  reboot has been provided. In this scenario, the reset reason will be
  [`kMfltRebootReason_Unknown`](components/panics/include/memfault/panics/trace_reason_types.h#L16)

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
  [`memfault_log_boot()`](https://github.com/memfault/memfault/blob/ccoleman/stg/sdk/embedded/components/core/include/memfault/core/log.h#L30-L38)
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
  `memfault/panics/reboot_tracking.h` for more details and
  https://mflt.io/2QlOlgH for a step-by-step setup tutorial.
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
