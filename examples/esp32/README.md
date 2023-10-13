# Memfault for ESP32

This example application shows an integration with the ESP-IDF v5.0.2 SDK where
a saved coredump is posted to the Memfault cloud for analysis.

If you already have an ESP-IDF project based on the v5.x, v4.x, or v3.x SDK, a step by
step getting started guide can be found [here](https://mflt.io/esp-tutorial).

The Memfault SDK has been tested to be compatible with these versions of
ESP-IDF:

- v3.x release series
- v4.x release series
  v5.x release series through v5.0

Other versions may be also be compatible but have not been verified by Memfault.

The memfault SDK hooks into the pre-existing mechanism for
[saving coredumps](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/core_dump.html).
We save the Memfault coredump data in the exact same coredump partition the
default implementation would use in the Memfault coredump format. This yields
several main benefits:

- The memory regions which are collected collected is highly configurable (via
  `memfault_platform_coredump_get_regions()`). This means all of RAM can be
  collected or just key globals and statics depending on your use case. In the
  Memfault cloud you will be able to inspect any of the variables collected,
  just like you would in an IDE or GDB.
- FreeRTOS thread recovery and traversal can take place in the Memfault cloud.
  By moving the recovery off the device, the fault handling logic is simplified
  and it's more likely for data to be recovered in the case where the crash
  happened as a result of memory corruption.
- Data can be posted directly from device to Memfault cloud for deduplication
  and analysis

The demo app is tested on the
[ESP-WROVER-KIT](https://www.espressif.com/en/products/hardware/esp-wrover-kit/overview).
The instructions below assume this development board is being used.

## Getting started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

We assume you have a working setup for the
[v5.0.2 SDK](https://docs.espressif.com/projects/esp-idf/en/v5.0.2/):

- have a version of CMAKE installed
- installed the xtensa
  [toolchain](https://docs.espressif.com/projects/esp-idf/en/v5.0.2/get-started/index.html#setup-toolchain)
  and added it to your path

<a name="adding-memfault"></a>

### Adding Memfault to the ESP-IDF SDK

1. Delete the dummy esp-idf directory (if present) and clone a copy of the
   v5.0.2 SDK.

   ```bash
   cd examples/esp32/
   rm -rf esp-idf
   git clone -b v5.0.2 --recursive https://github.com/espressif/esp-idf.git esp-idf
   cd esp-idf
   export IDF_TOOLS_PATH=$(pwd)
   # you may need to install the sdk tools by running ./install.sh here
   source ./export.sh
   ```

1. Install the memfault build id utility:

   ```bash
   pip install mflt-build-id
   ```

That's it! You should be good to go!

### Memfault Project Key

An API key will need to be configured for Memfault HTTP client to communicate
with Memfault's web services. Go to https://app.memfault.com/, navigate to the
project you want to use and select 'Settings'. Copy the 'Project Key', and
configure it; either by running `idf.py menuconfig` and setting the
`MEMFAULT_PROJECT_KEY` config value, or by inserting to `sdkconfig.defaults`:

```kconfig
CONFIG_MEMFAULT_PROJECT_KEY="<YOUR PROJECT KEY>"
```

> Note: when doing a clean build, or a build in CI, another option is to place
> the Project Key in a second `sdkconfig.defaults` file, for example:
>
> ```bash
> ❯ echo CONFIG_MEMFAULT_PROJECT_KEY=\"<YOUR PROJECT KEY>\" > sdkconfig.extra
> ❯ idf.py build -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.extra"
> ```

### Building the demo app

using pyinvoke:

```bash
invoke esp32.build
```

or use the Espressif command:

```bash
cd examples/esp32/apps/memfault_demo_app && idf.py build
```

### Flashing the demo app

```bash
invoke esp32.flash
```

or use the Espressif command:

```bash
cd examples/esp32/apps/memfault_demo_app && idf.py -p <console port, i.e -p /dev/ttyUSB1> flash
```

### Console

```bash
invoke esp32.console
```

or

```bash
miniterm.py ftdi://ftdi:2232/2 115200
```

## Using the demo app

The [demo app](https://mflt.io/demo-cli) is a simple console based app that has
commands to cause a crash in several ways. Once a coredump is captured, it can
be sent via WiFi to Memfault's web services to get analyzed and desymbolicated.
The `memfault/http` component of the Memfault SDK is used to talk to Memfault's
web services.

Let's walk through this process step by step:

### Memfault Project Key

An API key will need to be baked into the demo app to enable it to communicate
with Memfault's web services. Go to https://app.memfault.com/, navigate to the
project you want to use and select 'Settings'. Copy the 'Project Key' and paste
it into `esp32/apps/memfault_demo_app/main/config.c`, replacing
`<YOUR PROJECT KEY HERE>` with your API key. Save the file and rebuild, reflash
the project and attach the debug console (see instruction above).

### Checking the device info

As a sanity check, let's request the device info from the debug console, enter
`get_device_info` and press enter:

```plaintext
esp32> get_device_info
I (55239) mflt: S/N: 30AEA44AFF28
I (55239) mflt: SW type: esp32-main
I (55239) mflt: SW version: 1.0.0-dev
I (55239) mflt: HW version: esp32-proto
```

In the platform reference implementation for ESP32, the hardware version is
hard-coded to `esp32-proto`, software type is hard-coded to `esp32-main` and the
WiFi MAC address is used as serial number. You can change this to match your
requirements (see
[memfault_platform_device_info.c](apps/memfault_demo_app/main/memfault_platform_device_info.c))

### Causing a crash

Command `crash 1` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`:

```plaintext
esp32> crash 0
abort() was called at PC 0x400e54a3 on core 0

Backtrace: 0x4008fef0:0x3ffbc310 0x400900c7:0x3ffbc330 0x400e54a3:0x3ffbc350 0x400e5c0e:0x3ffbc370 0x4010a587:0x3ffbc390 0x400e4d09:0x3ffbc3b0 0x400d28b6:0x3ffbc3e0

Saving Memfault Coredump!
Rebooting...
```

Upon crashing, the coredump will be written to SPI flash. Note this can take a
several seconds. Once done, the board should reboot and show the console prompt
again.

To check whether the coredump has been captured, try running the `get_core`
command:

```plaintext
esp32> get_core

I (77840) mflt: Has coredump with size: 768
```

### Posting the coredump to Memfault for analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The ESP32 SDK demo app symbol file can be found in the build folder:

`apps/memfault_demo_app/build/memfault-esp32-demo-app.elf.memfault_log_fmt`

> Note: the file to be uploaded is
> `memfault-esp32-demo-app.elf.memfault_log_fmt`, _not_
> `memfault-esp32-demo-app.elf`, when using compact logs!

This ELF file contains the symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Post coredump

To post the collected crash to the Memfault cloud, first join a WiFi network:

```plaintext
esp32> join <SSID> <PASSWORD>
I (116450) connect: Connecting to '<SSID>'
```

Then post the data:

```plaintext
esp32> post_chunks
I (12419) mflt: Posting Memfault Data...
I (12419) mflt: Result: 0
```

### Clearing a coredump

New coredumps are only written if the coredump storage area is not already
occupied. Typically coredumps are cleared once they have been posted to the
memfault cloud by using the `memfault_platform_coredump_storage_clear` function.
You can invoke this command from the cli by running `clear_core`.

### Checking coredump storage capacity

Memfault coredumps will be written to the `coredump` partition by default. Use
the `coredump_size` command to see the computed maximum coredump size, and the
available storage capacity:

```bash
esp32> coredump_size
coredump storage capacity: 262144B
coredump size required: 456988B
```

### Testing OTA

The following steps can be used to exercise OTA functionality:

1. Build and flash the default image per
   [the instructions above](#getting-started). Confirm by running
   `get_device_info` in the `idf.py monitor` console that the software version
   is `1.0.0-dev`:

   ```bash
   esp32> get_device_info
   I (8358) mflt: S/N: A8032AEBF8E4
   I (8358) mflt: SW type: esp32-main
   I (8358) mflt: SW version: 1.0.1-dev
   I (8358) mflt: HW version: esp32-proto
   ```

2. If not already done, post some data from the device to register it in your
   Memfault project's fleet:

   ```bash
   esp32> join <ssid> <password>
   ...
   I (294468) connect: Connected
   esp32> post_chunks
   D (26028) mflt: Posting Memfault Data
   D (33288) mflt: Posting Memfault Data Complete!
   I (33288) mflt: Result: 0
   ```

3. Use `idf.py menuconfig` to change the value of
   `MEMFAULT_DEVICE_INFO_SOFTWARE_VERSION` config variable to `"1.0.1"`, and
   rebuild (`idf.py build`), but don't load it to the device. We'll use this
   build as our OTA payload!

4. In the Memfault web app, go to "Software->OTA Releases" and create an OTA
   release for `1.0.1-dev`. Click "Add OTA Payload to Release", select
   `esp32-proto` as the "Hardware Version" ("Software Type" should auto fill to
   `esp32-main`), and upload the
   `apps/memfault_demo_app/build/memfault-esp32-demo-app.bin` file as the
   payload (note: this step can also be done with the `memfault` cli, see
   [here](https://mflt.io/release-mgmt) for more information)

5. Deploy the release to your device; add it to a cohort under "Fleet->Cohorts",
   and set the "Target Release" to the `1.0.1-dev` version just created.

6. Back to the esp32 console, run `memfault_ota_check` to confirm the release is
   available for our device:

   ```bash
   esp32> memfault_ota_check
   D (274688) mflt: Checking for OTA Update
   D (276338) mflt: OTA Update Available
   D (276338) mflt: Up to date!
   ```

7. Now trigger the OTA release. The board will download the release and install
   it to the inactive OTA slot, then reboot (example output below truncated for
   brevity):

   ```bash
   esp32> memfault_ota_perform
   D (1455118) mflt: Checking for OTA Update
   D (1456618) mflt: OTA Update Available
   I (1456628) mflt: Starting OTA download ...
   I (1457798) esp_https_ota: Starting OTA...
   I (1457798) esp_https_ota: Writing to partition subtype 16 at offset 0x1e0000
   I (1476958) esp_https_ota: Connection closed
   ...
   I (1477758) mflt: OTA Update Complete, Rebooting System
   ...
   Rebooting...
   ...
   ```

8. When the console is back up, confirm that the update was applied by checking
   the Software Version:

   ```bash
   esp32> get_device_info
   I (8358) mflt: S/N: A8032AEBF8E4
   I (8358) mflt: SW type: esp32-main
   I (8358) mflt: SW version: 1.0.1-dev
   I (8358) mflt: HW version: esp32-proto
   ```
