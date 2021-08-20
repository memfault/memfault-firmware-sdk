# Memfault for ESP32

This example application shows an integration with the ESP-IDF v3.3.5 SDK where
a saved coredump is posted to the Memfault cloud for analysis.

If you already have an ESP-IDF project based on the v4.x or v3.x SDK, a step by
step getting started guide can be found [here](https://mflt.io/esp-tutorial).

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
[v4.2.2 SDK](https://docs.espressif.com/projects/esp-idf/en/v4.2.2/):

- have a version of CMAKE installed
- installed the xtensa
  [toolchain](https://docs.espressif.com/projects/esp-idf/en/v4.2.2/get-started/index.html#setup-toolchain)
  and added it to your path

<a name="adding-memfault"></a>

### Adding Memfault to the ESP-IDF SDK

1. Delete the dummy esp-idf directory (if present) and clone a copy of the
   v4.2.2 SDK.

   ```bash
   cd examples/esp32/
   rm -rf esp-idf
   git clone -b v4.2.2 --recursive https://github.com/espressif/esp-idf.git esp-idf
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

### Building the demo app:

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
I (55239) mflt: SW version: 1.0.0
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
The ESP32 SDK demo app symbol file can be found at:
`/path/to/examples/esp32/apps/memfault_demo_app/build/memfault-esp32-demo-app.elf`

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
esp32> post_core
I (12419) mflt: Posting Memfault Data...
I (12419) mflt: Result: 0
```

### Clearing a coredump

New coredumps are only written if the coredump storage area is not already
occupied. Typically coredumps are cleared once they have been posted to the
memfault cloud by using the `memfault_platform_coredump_storage_clear` function.
You can invoke this command from the cli by running `clear_core`.
