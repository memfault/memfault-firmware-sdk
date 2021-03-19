## Memfault for Zephyr RTOS

This folder contains an example integration of the Memfault SDK using the port
provided in `ports/zephyr`

The demo was tested using the
[STM32L4 discovery board](https://docs.zephyrproject.org/2.0.0/boards/arm/disco_l475_iot1/doc/index.html?#st-disco-l475-iot01-b-l475e-iot01a)
but should work for any ARM Cortex-M based Zephyr target board.

For the purposes of this demo we will be using the v2.0 release of Zephyr
because WiFi/HTTP is not fully implemented on the Zephyr v1.14 LTS Release.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

### Setup

1. Clone the repo

```bash
$ git clone git@github.com:zephyrproject-rtos/zephyr.git --branch v2.0-branch zephyr
```

2. Setup a working zephyr environment. Step-by-step instructions can be found
   [here](https://docs.zephyrproject.org/2.0.0/getting_started/index.html#build-hello-world).
3. Initialize a new build environment:

```bash
$ west init -l zephyr/
$ west update
```

4. Apply patches to integrate Memfault SDK

```
$ cd zephyr
$ git apply ../../../ports/zephyr/v2.0/zephyr-integration.patch
```

5. Add memfault-firmware-sdk to `ZEPHYR_EXTRA_MODULES` in CMakeLists.txt

```diff
+ set(MEMFAULT_ZEPHYR_PORT_TARGET v2.0)
+ set(MEMFAULT_SDK_ROOT /path/to/memfault-firmware-sdk)
+ list(APPEND ZEPHYR_EXTRA_MODULES ${MEMFAULT_SDK_ROOT}/ports)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(memfault_demo_app)
```

### Memfault Project Key

An API key will need to be baked into the [demo app](https://mflt.io/demo-cli)
to enable it to communicate with Memfault's web services. Go to
https://app.memfault.com/, navigate to the project you want to use and select
'Settings'. Copy the 'Project Key' and paste it into
`$MEMFAULT_SDK/examples/zephyr/apps/memfault_demo_app/src/main.c`, replacing
`<YOUR PROJECT KEY HERE>` with your Project Key.

### Building the App

```
$ cd $MEMFAULT_SDK/examples/zephyr/
$ west build -b disco_l475_iot1 apps/memfault_demo_app
[...]
[271/271] Linking C executable zephyr/zephyr.elf
```

### Running the App

At this point you should be able to
[flash and run](https://docs.zephyrproject.org/2.0.0/getting_started/index.html#run-the-application-by-flashing-to-a-board)
the application

```
$ miniterm.py /dev/cu.usbmodem* 115200 --raw
uart:~$ help mflt
mflt - Memfault Test Commands
Subcommands:
  crash            :trigger a crash
  clear_core       :clear the core
  get_core         :gets the core
  get_device_info  :display device information
  print_chunk      :get next Memfault data chunk to send and print as a curl command
  post_chunk       :get next Memfault data chunk to send and POST it to the Memfault
                    Cloud
uart:~$ mflt get_device_info
<inf> <mflt>: S/N: DEMOSERIAL
<inf> <mflt>: SW type: zephyr-main
<inf> <mflt>: SW version: 1.15.0
<inf> <mflt>: HW version: disco_l475_iot1
```

### Causing a crash

Command `mflt crash 1` will trigger a hard fault due to a bad instruction fetch
at a non-existing address, `0xbadcafe`:

```
uart:~$ mflt crash 1
... etc ...
```

Upon crashing, a coredump will be written to noinit RAM.

To check whether the coredump has been captured, try running the `get_core`
command after the device reboots:

```
uart:~$ mflt get_core
<inf> <mflt>: Has coredump with size: 580
```

This confirms that a coredump of 580 bytes has been captured.

### Posting Memfault Data for Analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The ELF is located at `build/zephyr/zephyr.elf`. This .elf contains the symbols
(debug information) amongst other things.

[More information on creating Software Versions and uploading symbols can be found here](https://mflt.io/2LGUDoA).

#### Connect WiFi

Before we can send the coredump, we need to make sure WiFi is connected to the
internet. Type `wifi connect <SSID> <SSID_STRLEN> 0 <PASSWORD>` in the console.
You should see something like:

```
Connection requested
Connected
uart:~$
```

#### Post data

Once connected, type `mflt post_msg` and press enter. You should see:

```
uart:~$ mflt post_msg
<dbg> <mflt>: Memfault Message Post Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: Memfault Message Post Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: No more data to send
```

A coredump is now being processed by Memfault's services and will show up
shortly under Issues. If it does not, take a look at the FAQ in the `README.md`
in the root of the SDK.

#### Without WiFi

The network stack dependencies can also be disabled completely by using the
`CONFIG_MEMFAULT_HTTP_SUPPORT` Kconfig option.

When using a board that does not have a network stack enabled or for debug
purposes, the messages to push to the Memfault cloud can also be dumped from the
CLI using the `mflt print_chunk` command:

```
uart:~$ mflt print_chunk
echo \
[...]
| xxd -p -r | curl -X POST https://chunks.memfault.com/api/v0/chunks/DEMOSERIAL\
 -H 'Memfault-Project-Key:<YOUR_PROJECT_KEY>\
 -H 'Content-Type:application/octet-stream' --data-binary @- -i
```

You can copy and paste the output into a terminal to upload the captured data to
Memfault.
