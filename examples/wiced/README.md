# Memfault for WICED SDK

This folder contains support files to make it easy to integrate Memfault into a
WICED SDK based project.

The demo app is tested on a [BCM943364WCD1] evaluation board. The instructions
below also assume the BCM943364WCD1 board.

The library and demo app are tested with v6.2.0 of the WICED SDK, but may also
work with other versions.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

<a name="adding-memfault"></a>

### Adding Memfault to the WICED SDK

1. Download and install the WICED SDK 6.2.0.
2. Create a symlink from `WICED-SDK-6.2.0/43xxx_Wi-Fi/apps/memfault_demo_app` to
   `memfault_sdk/examples/wiced/apps/memfault_demo_app`. For example:
   ```bash
    $ cd WICED-SDK-6.2.0/43xxx_Wi-Fi/apps
    $ ln -s /path/to/memfault_sdk/examples/wiced/apps/memfault_demo_app memfault_demo_app
   ```
3. Create a symlink from `WICED-SDK-6.2.0/43xxx_Wi-Fi/libraries/memfault` to
   `memfault_sdk/examples/wiced/libraries/memfault`. For example:
   ```bash
   $ cd WICED-SDK-6.2.0/43xxx_Wi-Fi/libraries
   $ ln -s /path/to/memfault_sdk/examples/wiced/libraries/memfault memfault
   ```
4. Optionally, to make the make the invoke tasks work, add a symlink inside the
   folder `memfault_sdk/examples/wiced/wiced_sdk` called `43xxx_Wi-Fi`, pointing
   to where you have installed the SDK. The final folder hierarchy should look
   like:
   ```
   - examples
     - wiced
       - wiced_sdk
         - 43xxx_Wi-Fi
           - apps
           - doc
           - ... etc ...
   ```

<a name="modifications"></a>

### Modifications to the WICED SDK

The WICED SDK defines a hard fault handler that Memfault needs to override in
order to capture a coredump when a hard fault happens. To do this, find the
`HardFaultException` function and comment it out. For ARM Cortex M4 targets, it
is defined in
`WICED-SDK-6.2.0/43xxx_Wi-Fi/WICED/platform/ARM_CM4/hardfault_handler.c`. If you
do not remove this function from the SDK code, the project will fail to link
with error "multiple definition of `HardFaultException'".

### Building the demo app

The SDK should now be able to automatically find the Memfault components and
build the [demo app](https://mflt.io/demo-cli). Now you can build the demo app:

using pyinvoke:

```bash
$ cd memfault_sdk
$ invoke wiced.build
```

or

```bash
$ cd /path/to/WICED-SDK-6.2.0/43xxx_Wi-Fi/
$ ./make memfault_demo_app-BCM943364WCD1-SDIO-debug
```

### Flashing the demo app

using pyinvoke:

```bash
$ cd memfault_sdk
$ invoke wiced.flash
```

or

```bash
$ cd /path/to/WICED-SDK-6.2.0/43xxx_Wi-Fi/
$ ./make memfault_demo_app-BCM943364WCD1-SDIO-debug download download_apps run
# You'll also want to clear the DEBUGEN bit ... otherwise a crash will trigger a breakpoint and the
# system will hang. The easiest way to do this is by cycling power. The invoke command fixes this
# automagically by executing the "stm32f4xx.cpu cortex_m disconnect" openocd command
```

### Attaching the debug console (via UART)

using pyinvoke:

```bash
$ cd memfault_sdk
$ invoke wiced.console
```

or

```bash
$ miniterm.py --raw /dev/cu.usbserial-* 115200
```

You can now type commands and view debug log output. To get the list of
commands, type `help` and press enter:

```bash
$ invoke wiced.console
--- Miniterm on /dev/cu.usbserial-14101  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
help
Console Commands:
    ?
    help [<command> [<example_num>]]
        - Print help message or command example.
    $?
        - Print return value of last executed command.
    get_core
        - Get coredump info
    clear_core
        - Clear an existing coredump
    post_core
        - Post coredump to Memfault

 ... etc ...
```

To detach, press `ctrl+]` (or `cmd+]` on macOS).

<a name="using-demo-app"></a>

## Using the demo app

The demo app is a simple console based app that has commands to cause a crash in
several ways. Upon crashing, the `memfault/panics` component of the Memfault SDK
will capture a coredump and save it to the external SPI flash. Once a coredump
is captured, it can be sent via WiFi to Memfault's web services to get analyzed
and desymbolicated. The `memfault/http` component of the Memfault SDK is used to
talk to Memfault's web services.

Let's walk through this process step by step:

### Memfault Project Key

An Project Key will need to be baked into the demo app to enable it to
communicate with Memfault's web services. Go to https://app.memfault.com/,
navigate to the project you want to use and select 'Settings'. Copy the 'Project
Key' and paste it into `apps/memfault_demo_app/memfault_demo_app.c`, replacing
`<YOUR PROJECT KEY HERE>` with your Project Key. Save the file and rebuild,
reflash the project and attach the debug console (see instruction above).

### Checking the device info

As a sanity check, let's request the device info from the debug console, enter
`get_device_info` and press enter:

```
> get_device_info
S/N: 00A0507510F0
SW type: wiced-main
SW version: 1.0.0
HW version: wiced-proto
```

In the platform reference implementation for WICED, the hardware version is
hard-coded to `wiced-proto`, software type is hard-coded to `wiced-main` and the
WiFi MAC address is used as serial number. You can change this to match your
requirements (see
`libraries/memfault/platform_reference_impl/memfault_platform_device_info.c`).

### Causing a crash

Command `test_hardfault` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`:

```
> test_hardfault

Starting WICED vWiced_006.002.001.0002
Platform BCM943364WCD1 initialised
Started ThreadX v5.8
Initialising NetX_Duo v5.10_sp3
... etc ...
```

Upon crashing, the coredump will be written to SPI flash. Note this can take a
up to 15 seconds. Once done, the board should reboot and show the console prompt
again.

To check whether the coredump has been captured, try running the `get_core`
command:

```
> get_core
Has coredump with size: 131312
```

This confirms that a coredump of 131312 bytes (the entire SRAM contents) has
been captured to SPI flash.

### Posting the coredump to Memfault for analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
When the WICED SDK builds the app, it creates an .elf file at
`/path/to/WICED-SDK-x.x.x/43xxx_Wi-Fi/build/memfault_demo_app-BCM943364WCD1-SDIO-release/binary/memfault_demo_app-BCM943364WCD1-SDIO-debug.elf`.
This .elf contains the symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Connect WiFi

Before we can send the coredump, we need to make sure WiFi is connected to the
internet. Type `join <SSID> wpa2 <PASSWORD>` in the console. You should see
something like:

```Joining : <SSID>
Successfully joined : <SSID>
Obtaining IPv4 address via DHCP
DHCP CLIENT hostname WICED IP
IPv4 network ready IP: 192.168.1.2
Setting IPv6 link-local address
IPv6 network ready IP: FE80:0000:0000:0000:02A0:50FF:FE75:10F0
```

#### Post coredump

Once connected, type `post_core` and press enter. You should see:

```Posting coredump...
Result: 0
```

The coredump is now being processed by Memfault's services and will show up
shortly under Issues. If it does not, take a look at the FAQ in the `README.md`
in the root of the SDK.

# Integrating into existing WICED projects

Adding Memfault to existing WICED projects is relatively simple. The Memfault
SDK comes with .mk files that are compatible with WICED SDK's components. This
has been tested with WICED SDK v6.2.0.

## Basic integration steps

- Add symlinks and necessary modifications to the WICED SDK. See
  [Adding Memfault to the WICED SDK](#adding-memfault) above for details.

- Add dependencies to Memfault's components in your app's .mk file:
  ```
  $(NAME)_COMPONENTS := ... existing dependencies ... \
                      libraries/memfault/core \
                      libraries/memfault/demo \
                      libraries/memfault/http \
                      libraries/memfault/panics \
                      libraries/memfault/platform_reference_impl \
  ```
  The `demo` is not strictly necessary and should only be added if you are also
  using the `utilities/command_console` component from the WICED SDK. The `demo`
  component contains console command implementation to test out various Memfault
  SDK features. See [Using the demo app](#using-demo-app) for an impression of
  these commands.
- Add configuration and initialization code:

```
// main.c

// Find your key on https://app.memfault.com/ under 'Settings':
sMfltHttpClientConfig g_mflt_http_client_config = {
    .api_key = "<YOUR PROJECT KEY HERE>",
};

void application_start(void) {
  wiced_init();
  memfault_platform_boot();

  ... your code ...
}

```

- To hook up the console commands, please refer to the `memfault_demo_app.c`
  example.
- The component `libraries/memfault/platform_reference_impl` contain
  implementations of platform dependencies that the Memfault SDK relies upon.
  They serve as a good example and starting point, but it's very likely you will
  need to customize some of the files to your use case. For example, by the
  software type defaults to `"wiced-main"`, the software version defaults to
  `"1.0.0"` and the hardware version defaults to `"wiced-proto"`. This needs to
  be changed to use the mechanisms you already have in place in your project to
  get the software type, software version and hardware version. Please refer to
  the `memfault_sdk/components/*/README.md` files to learn more about the
  customization options for each component.

# FAQ

If you run into any issues, please do not hesitate to reach out for help.

- I'm getting a linker error "multiple definition of
  `HardFaultException'" after adding`libraries/memfault/http` as a dependency to
  my project.

  - Make sure to comment out the `HardFaultException` function from the WICED
    SDK. See [Modifications to the WICED SDK](#modifications) above.

[bcm943364wcd1]:
  https://www.cypress.com/documentation/development-kitsboards/bcm943364wcd1evb-evaluation-and-development-kit
