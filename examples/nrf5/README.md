# Memfault for nRF5_SDK

This folder contains support files to make it easy to integrate Memfault into a
nRF5 SDK based project.

The demo app is tested on the [nRF52840 DK] -- PCA10056 evaluation board. The
instructions below assume this development board is being used and that the [nRF
Command Line Tools] have been installed.

The demo app is tested with v15.2.0 of the [nRF52 SDK], but should integrate in
a similar manner with other versions as well.

## Getting started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

<a name="adding-memfault"></a>

### Adding Memfault to the nRF5 SDK

1. Download the [v15.2.0 nRF5 SDK]
2. Delete the nrf5_sdk directory (if present) and create a symlink between that
   and the nRF5 SDK. For example

```bash
$ rm -rf nrf5_sdk
$ ln -s path/to/nrf5_sdk/ nrf5_sdk
```

<a name="modifications"></a>

### Modifications to the nRF5 SDK

The nRF5 SDK defines a couple assert macros that Memfault needs to override in
order to capture a coredump at the exact location an assert is hit. To do this,
the `app_error.h` is overriden at
`libraries/memfault/platform_reference_impl/sdk_overrides/app_error.h`. You
simply need to move the original file from the sdk to pick up this definition.
Otherwise, you will see an error like:

```
../../libraries/memfault/platform_reference_impl/sdk_overrides/app_error.h:50:2: error: #error "Please rename or remove components/libraries/util/app_error.h from the NRF SDK, as the Memfault SDK needs to override this header file."
```

Example fix:

```bash
  $ mv nrf5_sdk/components/libraries/util/app_error.h nrf5_sdk/components/libraries/util/app_error.h.overriden
```

### Update Demo App with your Memfault Project Key

A Project Key will need to be baked into the
[demo app](https://mflt.io/demo-cli) to enable it to communicate with Memfault's
web services. Go to https://app.memfault.com/, navigate to the project you want
to use and select 'Settings'. Copy the 'Project Key' and paste it into
`apps/memfault_demo_app/src/cli.c`, replacing `<YOUR PROJECT KEY HERE>` with
your Project Key.

### Building the demo app

You should now be able to compile the demo app with the Memfault components
included! To build:

using pyinvoke:

```bash
$ invoke nrf.build
```

or

```bash
$ cd /path/to/examples/nrf5/apps/memfault_demo_app/
$ export GNU_INSTALL_ROOT=/path/to/arm_toolchain_dir && make
```

### Flashing the demo app

_NOTE_: If you were previously using the dev board for something else, it's
recommended to put it into a clean state by performing a full erase

using pyinvoke:

```bash
$ invoke nrf.eraseflash
```

or

```bash
nrfjprog --eraseall
```

Once an erase has been performed you can flash the compiled demo app:

using pyinvoke:

```bash
# In one terminal start a gdbserver
$ invoke nrf.gdbserver

# In another terminal flash the binary
$ invoke nrf.flash
```

or

```bash
# In one terminal start a gdb server
JLinkGDBServer -if swd -device nRF52840_xxAA -speed auto -port 2331

# In another terminal flash the binary
$ cd /path/to/examples/nrf5/apps/memfault_demo_app/
$ make flash_softdevice
$ /path/to/arm-none-eabi-gdb[-py] --eval-command="target remote localhost:2331" --ex="mon reset" --ex="load" --ex="mon reset" --se=/path/to/examples/nrf5/apps/memfault_demo_app/build/memfault_demo_app_nrf52840_s140.out
```

once the flash has completed you can start the application by typing `continue`
in the gdb console

### Attaching the debug console (via SEGGER RTT)

You need to have started the GDBServer as described in the Flashing the demo
steps. Then from another terminal:

using pyinvoke:

```bash
$ invoke nrf.console
```

or

```bash
JLinkRTTClient -LocalEcho Off
```

## Using the demo app

The demo app is a simple console based app that has commands to cause a crash in
several ways. Upon crashing, the `memfault/panics` component of the Memfault SDK
will capture a coredump and save it to the internal device flash. For the
purposes of this demo, once a coredump is captured, it can be dumped out over
the console and posted to the Memfault web services. In a real world
environment, the data can be sent over BLE using a Gatt Service to a gateway
device (i.e a mobile phone) and posted to the Memfault web services in the same
way.

Let's walk through the coredump process step by step:

### Checking the device info

As a sanity check, let's request the device info from the debug console, enter
`get_device_info` and press enter:

```
> get_device_info
<info> app: MFLT: S/N: C9DB1227DEAE6A52
<info> app: MFLT: SW type: nrf-main
<info> app: MFLT: SW version: 1.0.0
<info> app: MFLT: HW version: nrf-proto
```

In the platform reference implementation for nRF, the hardware version is
hard-coded to `nrf-proto`, software type is hard-coded to `nrf-main` and the
NRF5 `DEVICEID` is used as serial number. You can change this to match your
requirements (see
`libraries/memfault/platform_reference_impl/memfault_platform_device_info.c`).

### Causing a crash

Command `test_hardfault` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`.

```
> test_hardfault
... etc ...
```

Upon crashing, the coredump will be written to internal NOR flash. Note this can
take a up to 15 seconds. Once done, you should see gdb hit a breakpoint and then
you can `next` through it to reboot:

```
Program received signal SIGTRAP, Trace/breakpoint trap.
memfault_platform_halt_if_debugging () at ../../libraries/memfault/platform_reference_impl/memfault_platform_core.c:13
13    NRF_BREAKPOINT_COND;
(gdb) next
memfault_platform_reboot () at ../../libraries/memfault/platform_reference_impl/memfault_platform_core.c:19
19    NVIC_SystemReset();
(gdb) continue
Continuing.
```

To check whether the coredump has been captured, try running the `get_core`
command:

```
rtt_cli:~$ get_core
<info> app: MFLT: Has coredump with size: 8448
```

This confirms that a coredump of 8448 bytes (the entire space allocated for the
stack) has been captured to internal flash.

### Posting the coredump to Memfault for analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The nRF SDK demo app symbol file can be found at:
`/path/to/examples/nrf5/apps/memfault_demo_app/build/memfault_demo_app_nrf52840_s140.out`

This ELF file contains the symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Post coredump

In a production environment the collected coredump can be posted to Memfault's
services via a gateway device with a connection to the internet. For example,
the data can be sent over BLE to a mobile application which will do the post.
Memfault has SDKs for different types of gateways (i.e iOS, Android) to
re-assemble data sent over a BLE link and post. If you are interested in these
implementations, please don't hesitate to reach out!

For the purposes of this demo, we will just grab the core information from the
cli using the `export` command. It should look something like:

```
rtt_cli:~$ export
<info> app: MFLT: MC:CAKmAgIDAQpobnJmLW1haW4JbDEuMC4wKzU1ZmMxMwZocGNhMTAwNTYEowEIBAQFAH/0:
```

You can copy & paste this output into the "Chunks Debug" view in the Memfault UI
or upload using the [desktop CLI tool](https://mflt.io/chunk-data-export). The
coredump is now being processed by Memfault's services and will show up shortly
under Issues. If it does not, take a look at the FAQ in the `README.md` in the
root of the SDK.

### Clearing a coredump

New coredumps are only written if the coredump storage area is not already
occupied. Typically coredumps are cleared once they have been posted to the
memfault cloud by using the `memfault_platform_coredump_storage_clear` function.
You can invoke this command from the cli by running `clear_core`.

# Integrating into existing NRF52 projects

Adding Memfault to existing NRF52 projects is relatively simple. The Memfault
SDK comes with an example of what adding Memfault to an nRF SDK Makefile would
look like and has been tested with nRF SDK v15.2.0.

## Basic integration steps

- Add Memfault component dependencies to your project. An example of how you can
  add Memfault to a typical nRF52 SDK based project can be found in the top of
  the `apps/memfault_demo_app/Makefile` in the section titled "Memfault SDK
  integration". The Makefile commands:
  - Collect the Memfault SDK components under `components`. The Memfault SDK has
    libraries for the features being used under `components/*`. The `demo`
    component used in the example app is not strictly necessary and should only
    be added if you are interested in adding the same cli commands to your
    project
  - Picks up the include paths for each component and adds them to the nRF5
    build system
  - Picks up platform specific reference implementations needed for the Memfault
    SDK
- Add configuration and initialization code:

```
// main.c

int main(void) {
  [... other initialization code ...]
  memfault_platform_boot();
  [... application code ...]
```

- If your project doesn't already include them you may need to add a few files
  from the nRF SDK to your project such as `modules/nrfx/hal/nrf_nvmc.c` which
  is used by the reference platform implementation
- NOTE: The component `libraries/memfault/platform_reference_impl` contain
  implementations of platform dependencies that the Memfault SDK relies upon.
  They serve as a good example and starting point, but it's very likely you will
  need to customize some of the files to your use case. For example, by the
  software type defaults to `"nrf-main"`, the software version defaults to
  `"1.0.0"` and the hardware version defaults to `"nrf-proto"`. This needs to be
  changed to use the mechanisms you already have in place in your project to get
  the software type, software version and hardware version. Please refer to the
  `memfault_sdk/components/*/README.md` files to learn more about the
  customization options for each component.
- To save coredumps in internal flash, you will need to budget some space to
  store them. You can find more details in the top of
  `libraries/memfault/platform_reference_impl/memfault_platform_coredump.c` but
  basically it just involves adding a new section to your .ld script. You can
  also configure how much of ram you want to capture in this file. The demo app
  is only collecting the active stack using the
  `MEMFAULT_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY` define but if space is
  available its great to capture all of RAM for the best amount of detail when
  doing post-mortem analysis

[v15.2.0 nrf5 sdk]:
  https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip
[nrf52840 dk]:
  https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK
[nrf52 sdk]: https://developer.nordicsemi.com/nRF5_SDK/
[http api to upload symbols files]:
  https://docs.memfault.com/?version=latest#ca2a72d2-69ef-4703-98bf-60621738091a
[nrf command line tools]:
  https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools
